// Copyright Carl Kelso, 2023, All rights reserved
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <list>

std::list<uint32_t> found;

enum {
    PRINT_COMPARISONS = 1 << 0
};

uint32_t print_flags = 0;

uint32_t clz( uint32_t value ) {
    return __builtin_clz( value );
}

uint32_t get_lsb( uint32_t value ) {
    return value & -value;
}

uint32_t get_msb( uint32_t value ) {
    return 1 << ( 31 - __builtin_clz( value ) );
}

int get_shift( uint32_t value ) {
    uint32_t lowest = get_lsb( value );
    return 31 - __builtin_clz( lowest );
}

int get_msb_shift( uint32_t value ) {
    return 31 - __builtin_clz( value );
}

char to_char( uint32_t value ) {
    uint32_t shift = get_shift( value );
    return 'A' + shift;
}

void print_group( uint32_t value ) {
    while ( value ) {
        uint32_t lowest = get_lsb( value );
        printf("%c", to_char(lowest) );
        value &= ~lowest;
    }
    printf( "\n" );
}

int count_non_zeros( uint32_t value ) {
    static uint32_t mask[] = {
        0x55555555,
        0x33333333,
        0x0F0F0F0F,
        0x00FF00FF,
        0x0000FFFF
    };
    for ( int i = 0; i < 5; ++i ) {
        uint32_t split = value & mask[i];
        value = ( value >> ( 1 << i ) ) & mask[i];
        value += split;
    }
    return value;
}

bool check_next_collides(
        const std::list<uint32_t> & found,
        uint32_t current,
        uint32_t step_mask,
        uint32_t t ) {
    if ( print_flags & PRINT_COMPARISONS ) {
        printf( "Checking current is valid: " );
        print_group( current );
    }

    for ( const auto & found_entry : found ) {
        if ( print_flags & PRINT_COMPARISONS ) {
            printf( "Comparing " );
            print_group( found_entry );
        }
        if ( count_non_zeros( found_entry & ( current | step_mask ) ) >= t ) {
            return true;
        }
    }
    return false;
}

bool advance_mask( uint32_t & mask, uint32_t v ) {
    int number_of_bits = count_non_zeros( mask );
    printf( "Advancing 0x%0X ", mask );
    for ( int i = 0; i < number_of_bits; ++i ) {
        int msb = get_msb( mask );
        uint32_t max_bit = 1 << ( v - 1 - i );
        mask ^= msb; // Delete the highest bit
        msb <<= 1;
        if ( msb < max_bit ) {
            mask |= ((2 << i) - 1) * msb;
            printf( "to 0x%0X, msb: 0x%0X, max_bit: 0x%0X\n", mask, msb, max_bit );
            return true;
        }
    }
    printf( "Failed\n" );
    return false;
}

int main( int argc, char ** argv ) {

    if ( argc < 4 ) {
        fprintf( stderr, "Please use `%s <v> <k> <t>` where v > k > t\n", argv[0] );
        return 1;
    }

    int v = atoi( argv[1] );
    int k = atoi( argv[2] );
    int t = atoi( argv[3] );

    if ( v < k || k < t || t < 1 || v > 26 ) {
        fprintf( stderr, "26 >= v > k > t >= 1 is required\n" );
        return 2;
    }

    printf( "Naively calculating steiner group S(%d,%d,%d)\n", v, k, t );

    uint32_t valid_mask = (1 << v) - 1;
    printf( "Using alphabet %c to %c, valid mask: 0x%0X\n", 'A', 'A' + v - 1, valid_mask);

    // Initial field
    uint32_t current = (1 << k) - 1;
    found.push_back( current );
    int steps = 1;

    // We need to be at least this far away from the previous entry or we're
    // not valid.
    int step_size = k - t + 1;
    uint32_t step_mask = ( 1 << step_size ) - 1 << (t - 1);

    current ^= step_mask;

    int new_min = 0;

    printf( "Step 1 size: %d, current: 0x%0X, new mask: 0x%0X\n", step_size, current, step_mask );
    print_group( current | step_mask );

    bool test2 = true; // True if we can advance our mask
    // Construct the next field by advancing a step.
    do {
        for ( new_min = get_msb_shift( step_mask ) + 1;
                new_min <= v - step_size;
                new_min = get_msb_shift( step_mask ) + 1 ) {
            ++steps;
            step_mask = ( ( 1 << step_size ) - 1 ) << new_min;
            bool test = check_next_collides( found, current, step_mask, t );
            test2 = true;
            while ( test && test2 ) {
                test2 = advance_mask( step_mask, v );
                test = check_next_collides( found, current, step_mask, t );
            }
            printf( "Test results: %c and %c\n", test ? 'T' : 'F', test2 ? 'T' : 'F' );
            if ( !test && test2 ) {
                found.push_back( current | step_mask );
                printf( "Step 2 size: %d, current: 0x%0X, new mask: 0x%0X\n", step_size, current, step_mask );
                print_group( current | step_mask );
                test2 = false;
            } else {
                break;
            }
        }
        printf( "Rolling over with new_min: %d, current: 0x%0X\n", new_min, current );

        if ( current ) {
            test2 = advance_mask( current , v - step_size );
            new_min = get_msb_shift( current );
            printf( "Advancing new_min: %d, current: 0x%0X\n", new_min, current );

            if ( test2 ) {
                step_mask = ( (1 << step_size) - 1 ) << get_msb_shift( current ) + 1;
                bool test = check_next_collides( found, current, step_mask, t );
                test2 = true;
                while ( test && test2 ) {
                    test2 = advance_mask( step_mask, v );
                    test = check_next_collides( found, current, step_mask, t );
                }
                if ( !test && test2 ) {
                    found.push_back( current | step_mask );
                    printf( "Step 3 size: %d, current: 0x%0X, new mask: 0x%0X\n", step_size, current, step_mask );
                    print_group( current | step_mask );
                }
            }
        }
    } while ( test2 );

    printf( "Completed in %d steps with %zu entries\n", steps, found.size() );

    for ( auto & entry : found ) {
        print_group( entry );
    }

    // TODO - Confirm all subsets occur only once
    // Note, this current implementation fails in some interesting ways.  It may be preferable to
    // iterate through fully independent sets first, and then fill in the gaps.
}
