// #pragma once

// #include <kaminari/packers/packer.hpp>


// namespace kaminari
// {
//     template <typename Packer>
//     class queue : public Packer
//     {
//     public:
//         inline void clear();
//     };


//     template <typename Packer>
//     inline void queue<Packer>::clear()
//     {
//         static_cast<packer<Packer, typename Packer::pending_t>&>(*this).clear();
//     }
// }
