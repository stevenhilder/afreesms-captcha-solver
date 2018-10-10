#include <stdio.h> /* for stderr, fprintf(), printf() */
#include <stdlib.h> /* for EXIT_FAILURE, EXIT_SUCCESS */

#include "solver.h"

int main(int argc, char** argv) {
    int retval = EXIT_FAILURE;

    /* Check that solver was called correctly */
    if (argc != 2 || *argv[1] == 0) {
        fprintf(stderr, "Usage: %s <FILENAME>\n", argv[0]);

    /* Run solver */
    } else {
        struct solver_context solver;
        if (solver_initialize(&solver, argv[0], argv[1])) {
            if (solver_preprocess(&solver) && solver_solve(&solver)) {
                printf("%zu\n", solver.solved_value);
                retval = EXIT_SUCCESS;
            }
            solver_terminate(&solver);
        }
    }
    return retval;
}
