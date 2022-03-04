#include <catch2/catch_all.hpp>

#include <kaminari/packers/immediate_packer.hpp>


SCENARIO("a packer processes many accumulated data")
{
    GIVEN("A single populated packer")
    {

        WHEN("it processes")
        {
            server->stop();

            THEN("size is whithin constraints")
            {
            }
        }
    }
}
