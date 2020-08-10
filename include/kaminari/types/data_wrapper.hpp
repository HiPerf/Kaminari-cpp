#pragma once

#include <inttypes.h>


namespace kaminari
{
    struct data_wrapper;
}


extern void release_data_wrapper(::kaminari::data_wrapper* x);


namespace kaminari
{
    struct data_wrapper
    {
        friend inline void intrusive_ptr_add_ref(data_wrapper* x);
        friend inline void intrusive_ptr_release(data_wrapper* x);

    public:
        data_wrapper() :
            _refs(0)
        {}
        
        // TODO(gpascualg): Magic numbers, 500 is ~UDP max size
        uint8_t data[500];
        uint16_t size;

    private:
        uint8_t _refs;
    };


    inline void intrusive_ptr_add_ref(::kaminari::data_wrapper* x)
    {
        x->_refs += 1;
    }

    inline void intrusive_ptr_release(::kaminari::data_wrapper* x)
    {
        if (--x->_refs == 0)
        {
            release_data_wrapper(x);
        }
    }
}
