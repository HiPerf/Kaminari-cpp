#pragma once

#include <kaminari/addons/peer_based_phase_sync.hpp>

#include <function2/function2.hpp>

#include <inttypes.h>
#include <type_traits>
#include <chrono>
#include <thread>
#include <mutex>


namespace kaminari
{
    template<typename Test, template<typename...> class Ref>
    struct is_specialization : std::false_type {};

    template<template<typename...> class Ref, typename... Args>
    struct is_specialization<Ref<Args...>, Ref> : std::true_type {};

    template <typename T>
    concept chrono_duration = is_specialization<T, std::chrono::duration>::value;

    template <chrono_duration base_time, uint64_t expected_tick_rate>
    class phase_sync
    {
        using clock_t = std::chrono::steady_clock;

    public:
        phase_sync();
        phase_sync(phase_sync<base_time, expected_tick_rate>&& other);
        phase_sync<base_time, expected_tick_rate>& operator=(phase_sync<base_time, expected_tick_rate>&& other);

        ~phase_sync();

        template <typename C>
        void on_receive_packet(C* client, uint16_t last_server_id);

        template <typename C> void register_oneshot_early(C&& callback);
        template <typename C> void register_oneshot(C&& callback);
        template <typename C> void register_oneshot_late(C&& callback);

        template <typename C> void register_recurrent_early(C&& callback);
        template <typename C> void register_recurrent(C&& callback);
        template <typename C> void register_recurrent_late(C&& callback);

    private:
        void start();

        inline void call_all(std::vector<fu2::unique_function<void()>>& vector);

    private:
        clock_t::time_point _tick_time;
        clock_t::time_point _next_tick;
        float _integrator;
        float _adjusted_integrator;
        uint16_t _last_processed_packet;

        std::mutex _mutex;
        std::vector<fu2::unique_function<void()>> _oneshot_early;
        std::vector<fu2::unique_function<void()>> _oneshot;
        std::vector<fu2::unique_function<void()>> _oneshot_late;
        std::vector<fu2::unique_function<void()>> _recurrent_early;
        std::vector<fu2::unique_function<void()>> _recurrent;
        std::vector<fu2::unique_function<void()>> _recurrent_late;

        bool _running;
        std::thread _thread;
    };

    template <chrono_duration base_time, uint64_t expected_tick_rate>
    phase_sync<base_time, expected_tick_rate>::phase_sync() :
        _tick_time(clock_t::now()),
        _next_tick(clock_t::now() + base_time(expected_tick_rate)),
        _integrator(expected_tick_rate),
        _adjusted_integrator(expected_tick_rate),
        _last_processed_packet(0),
        _mutex(),
        _running(true),
        _thread(&phase_sync<base_time, expected_tick_rate>::start, this)
    {}

    template <chrono_duration base_time, uint64_t expected_tick_rate>
    phase_sync<base_time, expected_tick_rate>::~phase_sync()
    {
        _running = false;
        _thread.join();
    }

    template <chrono_duration base_time, uint64_t expected_tick_rate>
    phase_sync<base_time, expected_tick_rate>::phase_sync(phase_sync<base_time, expected_tick_rate>&& other) :
        _tick_time(std::move(other._tick_time)),
        _next_tick(std::move(other._next_tick)),
        _integrator(other._integrator),
        _adjusted_integrator(other._adjusted_integrator),
        _last_processed_packet(other._last_processed_packet),
        _running(other._running)
    {
        std::lock_guard lk1(other._mutex);
        std::lock_guard lk2(_mutex);

        _oneshot_early = std::move(other._oneshot_early);
        _oneshot = std::move(other._oneshot);
        _oneshot_late = std::move(other._oneshot_late);
        _recurrent_early = std::move(other._recurrent_early);
        _recurrent = std::move(other._recurrent);
        _recurrent_late = std::move(other._recurrent_late);

        // Move thread last
        _thread = std::move(other._thread);
    }

    template <chrono_duration base_time, uint64_t expected_tick_rate>
    phase_sync<base_time, expected_tick_rate>& phase_sync<base_time, expected_tick_rate>::operator=(phase_sync<base_time, expected_tick_rate>&& other)
    {
        _tick_time = std::move(other._tick_time);
        _next_tick = std::move(other._next_tick);
        _integrator = other._integrator;
        _adjusted_integrator = other._adjusted_integrator;
        _last_processed_packet = other._last_processed_packet;
        _running = other._running;

        std::lock_guard lk1(other._mutex);
        std::lock_guard lk2(_mutex);
        _oneshot_early = std::move(other._oneshot_early);
        _oneshot = std::move(other._oneshot);
        _oneshot_late = std::move(other._oneshot_late);
        _recurrent_early = std::move(other._recurrent_early);
        _recurrent = std::move(other._recurrent);
        _recurrent_late = std::move(other._recurrent_late);

        // Move thread last
        _thread = std::move(other._thread);

        return *this;
    }

    template <chrono_duration base_time, uint64_t expected_tick_rate>
    template <typename C>
    void phase_sync<base_time, expected_tick_rate>::on_receive_packet(C* client, uint16_t last_server_id)
    {
        // Get current tick time
        auto time = _tick_time + base_time((uint64_t)_integrator);

        // More than one packet in between?
        //auto packet_diff = cx::overflow::sub(last_server_id, _last_processed_packet);
        uint16_t packet_diff = 1;
        _last_processed_packet = last_server_id;
        if (packet_diff > 1)
        {
            _next_tick += base_time((uint64_t)(_integrator * (packet_diff - 1)));
        }

        // Phase detector
        auto clock_err = time - _next_tick;
        float err = (float)std::chrono::duration_cast<base_time>(clock_err).count();

        // Loop filter
        // Integrator = 0.999f * Integrator + err;
        const float Ki = 1e-3f;
        _integrator = Ki * err + _integrator;

        // NCO
        _next_tick = time + base_time((uint64_t)_integrator);

        if constexpr (C::template has_stateful_callback<peer_based_phase_sync>())
        {
            _adjusted_integrator = _integrator + ((peer_based_phase_sync*)client)->superpackets_id_diff();
        }
        else
        {
            _adjusted_integrator = _integrator;
        }
    }

    template <chrono_duration base_time, uint64_t expected_tick_rate>
    template <typename C>
    void phase_sync<base_time, expected_tick_rate>::register_oneshot_early(C&& callback)
    {
        std::lock_guard lk(_mutex);
        _oneshot_early.emplace_back(callback);
    }

    template <chrono_duration base_time, uint64_t expected_tick_rate>
    template <typename C>
    void phase_sync<base_time, expected_tick_rate>::register_oneshot(C&& callback)
    {
        std::lock_guard lk(_mutex);
        _oneshot.emplace_back(callback);
    }

    template <chrono_duration base_time, uint64_t expected_tick_rate>
    template <typename C>
    void phase_sync<base_time, expected_tick_rate>::register_oneshot_late(C&& callback)
    {
        std::lock_guard lk(_mutex);
        _oneshot_late.emplace_back(callback);
    }

    template <chrono_duration base_time, uint64_t expected_tick_rate>
    template <typename C>
    void phase_sync<base_time, expected_tick_rate>::register_recurrent_early(C&& callback)
    {
        std::lock_guard lk(_mutex);
        _recurrent_early.emplace_back(callback);
    }

    template <chrono_duration base_time, uint64_t expected_tick_rate>
    template <typename C>
    void phase_sync<base_time, expected_tick_rate>::register_recurrent(C&& callback)
    {
        std::lock_guard lk(_mutex);
        _recurrent.emplace_back(callback);
    }

    template <chrono_duration base_time, uint64_t expected_tick_rate>
    template <typename C>
    void phase_sync<base_time, expected_tick_rate>::register_recurrent_late(C&& callback)
    {
        std::lock_guard lk(_mutex);
        _recurrent_late.emplace_back(callback);
    }

    template <chrono_duration base_time, uint64_t expected_tick_rate>
    void phase_sync<base_time, expected_tick_rate>::start()
    {
        while (_running)
        {
            _tick_time = clock_t::now();

            // Call callbacks
            {
                std::lock_guard lk(_mutex);

                call_all(_oneshot_early);
                call_all(_recurrent_early);

                call_all(_oneshot);
                call_all(_recurrent);

                call_all(_oneshot_late);
                call_all(_recurrent_late);

                // Clear oneshots
                _oneshot_early.clear();
                _oneshot.clear();
                _oneshot_late.clear();
            }

            std::this_thread::sleep_for(base_time((uint64_t)_adjusted_integrator));
        }
    }

    template <chrono_duration base_time, uint64_t expected_tick_rate>
    inline void phase_sync<base_time, expected_tick_rate>::call_all(std::vector<fu2::unique_function<void()>>& vector)
    {
        for (auto&& fnc : vector)
        {
            std::move(fnc)();
        }
    }
}
