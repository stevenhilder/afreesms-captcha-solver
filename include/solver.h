#ifndef SOLVER_H
#define SOLVER_H

#include <stdbool.h> /* for bool */
#include <sys/types.h> /* for size_t */

#include <MagickCore/MagickCore.h> /* ImageMagick C API */

struct solver_context {
    Image* image;
    ImageInfo* image_info;
    CacheView* image_view;
    ExceptionInfo* exception;
    size_t solved_value;
};

/* Initializes the solver context and loads the specified file */
bool solver_initialize(struct solver_context* solver, char* executable_name, char* image_filename);

/* Performs image filtering to make the pixel values easier to interpret */
bool solver_preprocess(struct solver_context* solver);

/* Solves the CAPTCHA */
bool solver_solve(struct solver_context* solver);

/* Destroys the solver context and frees memory */
void solver_terminate(struct solver_context* solver);

#endif
