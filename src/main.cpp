// Copyright Carl Kelso, 2023, All rights reserved
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <list>

std::list<uint32_t> found;

enum {
    PRINT_COMPARISONS = 1 << 0,
    PRINT_ADVANCE_MASK = 1 << 1,
    PRINT_NEXT_CANDIDATE = 1 << 2,
    PRINT_BASE_MASKS = 1 << 3,
    PRINT_STEP_MASKS = 1 << 4,
    PRINT_CHECKS = 1 << 5,
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

void print_group( uint32_t value ) {
    while ( value ) {
        uint32_t lowest = get_lsb( value );
        printf("%c", to_char(lowest) );
        value &= ~lowest;
    }
    printf( "\n" );
}

// This advances the "next candidate" past anything that clearly can't be valid.
uint32_t get_next_candidate( uint32_t value, uint32_t consume_mask ) {
    if ( print_flags & PRINT_NEXT_CANDIDATE ) {
        printf( "Converting " ); print_group( value );
    }
    int bit_count = count_non_zeros( value );
    consume_mask &= ~value;
    uint32_t new_value_mask = 0;
    for ( int i = 0; i < bit_count; ++i ) {
        uint32_t next_bit = get_lsb( consume_mask );
        new_value_mask |= next_bit;
        consume_mask &= ~next_bit;
        if ( next_bit == 0 ) {
            new_value_mask = 0;
            break;
        }
    }
    if ( print_flags & PRINT_NEXT_CANDIDATE ) {
        printf( "   to      " ); print_group( new_value_mask );
    }
    return new_value_mask;
}

uint32_t reset_consume_mask( uint32_t current_step, int v ) {
    uint32_t low = get_msb( current_step ) << 1;
    uint32_t high = (1 << v);
    uint32_t consume_mask = 0;
    if ( high > low ) {
        consume_mask = high - low;
    }
    return consume_mask;
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

/// @returns true when successful
bool advance_mask( uint32_t & mask, uint32_t v ) {
    int number_of_bits = count_non_zeros( mask );
    if ( print_flags & PRINT_ADVANCE_MASK ) {
        printf( "Advancing 0x%0X ", mask );
    }
    for ( int i = 0; i < number_of_bits; ++i ) {
        int msb = get_msb( mask );
        uint32_t max_bit = 1 << ( v  - i );
        mask ^= msb; // Delete the highest bit
        msb <<= 1;
        if ( msb < max_bit ) {
            mask |= ((2 << i) - 1) * msb;
            if ( print_flags & PRINT_ADVANCE_MASK ) {
                printf( "to 0x%0X, msb: 0x%0X, max_bit: 0x%0X\n", mask, msb, max_bit );
            }
            return true;
        }
    }
    if ( print_flags & PRINT_ADVANCE_MASK ) {
        printf( "Failed\n" );
    }
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

    int step_size = k - t + 1;
    int base_size = k - step_size;
    int base_limit = v - step_size;

    int steps = 0;
    enum { step_limit = 25 };


    bool base_test = true;
    for ( uint32_t base_mask = ( 1 << base_size ) - 1;
          base_test;
          base_test = advance_mask( base_mask, base_limit ) ) {
        if ( print_flags & PRINT_BASE_MASKS ) {
            printf( "base_mask: " ); print_group( base_mask );
        }
        uint32_t step_min_bit = get_msb( base_mask ) << 1;
        uint32_t step_candidate_mask = ( 1 << v ) - step_min_bit;
        for ( uint32_t step_mask = step_min_bit * ( ( 1 << step_size ) - 1 );
              count_non_zeros( step_candidate_mask ) >= step_size;
              step_mask = get_next_candidate( step_mask, step_candidate_mask ) ) {
            if ( print_flags & PRINT_STEP_MASKS ) {
                printf( "step_candidate_mask: " ); print_group( step_candidate_mask );
                printf( "step_mask: " ); print_group( step_mask );
            }
            ++steps;
            if ( steps > step_limit ) {
                printf( "HIT STEP LIMIT!\n" );
                return -1;
            }
            bool test = check_next_collides( found, base_mask, step_mask, t );
            bool test2 = true;
            while ( test && test2 ) {
                if ( print_flags & PRINT_CHECKS ) {
                    printf( "checking:    " ); print_group( base_mask | step_mask );
                }
                test2 = advance_mask( step_mask, v );
                test = check_next_collides( found, base_mask, step_mask, t );
            }
            if ( print_flags & PRINT_CHECKS ) {
                printf( "checking:    " ); print_group( base_mask | step_mask );
            }
            if ( !test && test2 ) {
                found.push_back( base_mask | step_mask );
            }

            if ( !test2 ) {
                break;
            }
            step_candidate_mask &= ~step_mask;
        } // Deep search
    } // Base stepping


    printf( "Completed in %d steps with %zu entries\n", steps, found.size() );

    for ( auto & entry : found ) {
        print_group( entry );
    }

    // TODO - Confirm all subsets occur only once
    // Note, this current implementation fails in some interesting ways.  It may be preferable to
    // iterate through fully independent sets first, and then fill in the gaps.
}
