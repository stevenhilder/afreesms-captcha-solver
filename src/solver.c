#include <stdbool.h> /* for bool */
#include <stdio.h> /* for stderr, fputs() */
#include <string.h> /* for memset(), strcpy() */
#include <sys/types.h> /* for size_t, ssize_t */

#include <MagickCore/MagickCore.h> /* ImageMagick C API */

#include "solver.h"

/* Maps pixel values to 0 (white) and 1 (black), returns the sum for the specified pixel column */
static size_t solver_column_sum(struct solver_context* solver, size_t column) {
    size_t retval = 0;

    /* Retrieve pixel cursor for the specified column */
    register Quantum* pixel = GetCacheViewAuthenticPixels(solver->image_view, (ssize_t)column, 0, 1, solver->image->rows, solver->exception);

    /* Iterate over single-pixel rows */
    for (size_t row = 0; row < solver->image->rows; ++row) {

        /* Increment the sum as appropriate */
        retval += pixel[0] == 0 ? 1 : 0;

        /* Move to the next row */
        pixel += GetPixelChannels(solver->image);
    }
    return retval;
}

/* Crops the input image using the specified coordinates */
static bool solver_crop(struct solver_context* solver, ssize_t x, ssize_t y, size_t width, size_t height) {
    bool retval = false;

    /* Set up crop geometry */
    RectangleInfo geometry;
    geometry.x = x;
    geometry.y = y;
    geometry.width = width;
    geometry.height = height;

    /* Crop the image */
    Image* temp = NULL;
    if ((temp = CropImage(solver->image, &geometry, solver->exception)) == NULL) {
        CatchException(solver->exception);

    /* Success; destroy the uncropped image and continue with the cropped image */
    } else {
        DestroyImage(solver->image);
        solver->image = temp;
        retval = true;
    }
    return retval;
}

/* Converts the input image from color to grayscale */
static bool solver_grayscale(struct solver_context* solver) {
    bool retval = false;

    /* Perform the color channel transformation */
    if (GrayscaleImage(solver->image, Rec709LuminancePixelIntensityMethod, solver->exception) == MagickFalse) {
        CatchException(solver->exception);

    /* Success */
    } else {
        retval = true;
    }
    return retval;
}

/* Remap luminance values using the specified black point, white point and gamma */
static bool solver_level(struct solver_context* solver, const double black_point, const double white_point, const double gamma) {
    bool retval = false;

    /* Perform the "Levels" operation */
    if (LevelImage(solver->image, QuantumRange * black_point, QuantumRange * white_point, gamma, solver->exception) == MagickFalse) {
        CatchException(solver->exception);

    /* Success */
    } else {
        retval = true;
    }
    return retval;
}

/* Converts the input image to monochrome; ie. pixels will be either black or white; no shades of gray */
static bool solver_monochrome(struct solver_context* solver) {
    bool retval = false;

    /* Perform the transformation */
    if (SetImageType(solver->image, BilevelType, solver->exception) == MagickFalse) {
        CatchException(solver->exception);

    /* Success */
    } else {
        retval = true;
    }
    return retval;
}

/* Returns the appropriate digit for the specified segment signature */
static size_t solver_segment_value(size_t segment_signature) {
    switch (segment_signature) {
        case 101: return 1;
        case 338: return 2;
        case 381: return 3;
        case 336: return 4;
        case 358: return 5;
        case 375: return 6;
        case 301: return 7;
        case 407: return 8;
        case 424: return 9;
        default: return 0;
    }
}

/* Initializes the solver context and loads the specified file */
bool solver_initialize(struct solver_context* solver, char* executable_name, char* image_filename) {
    bool retval = false;

    /* Void all values in the solver_context struct */
    memset(solver, 0, sizeof (struct solver_context));

    /* Initialize MagickCore */
    MagickCoreGenesis(executable_name, MagickFalse);

    /* Allocate memory for ImageInfo struct, using default values */
    if ((solver->image_info = CloneImageInfo(NULL)) == NULL) {
        fputs("Unable to allocate memory for ImageInfo\n", stderr);

    /* Allocate memory for ExceptionInfo struct */
    } else if ((solver->exception = AcquireExceptionInfo()) == NULL) {
        fputs("Unable to allocate memory for ExceptionInfo\n", stderr);

    /* Load the input image file */
    } else {
        strcpy(solver->image_info->filename, image_filename);
        if ((solver->image = ReadImage(solver->image_info, solver->exception)) == NULL) {
            CatchException(solver->exception);

        /* Success */
        } else {
            retval = true;
        }
    }
    return retval;
}

/* Performs image filtering to make the pixel values easier to interpret */
bool solver_preprocess(struct solver_context* solver) {
    bool retval = false;
    if (

        /* Remove the bottom part of the image (it isn't part of the CAPTCHA) */
        solver_crop(solver, 0, 0, 76, 18)

        /* Discard RGB color information */
        && solver_grayscale(solver)

        /* Remap luminance for repeatable conversion to monochrome */
        && solver_level(solver, 0.4, 0.6, 0.3)

        /* Convert to monochrome */
        && solver_monochrome(solver)

    /* Success */
    ) {
        retval = true;
    }
    return retval;
}

/* Solves the CAPTCHA */
bool solver_solve(struct solver_context* solver) {
    bool retval = false;

    /* Retrieve cached pixel view */
    if ((solver->image_view = AcquireAuthenticCacheView(solver->image, solver->exception)) == NULL) {
        CatchException(solver->exception);

    /* Perform segmentation and calculate a unique signature for each digit */
    } else {
        bool capture_segment = false;
        size_t column_index_multiplier = 0;
        size_t segment_signature = 0;
        solver->solved_value = 0;

        /* Iterate over pixel columns */
        for (size_t column = 0; column < solver->image->columns; ++column) {
            size_t column_sum = solver_column_sum(solver, column);

            /* Check for empty columns (a column with 0 or 1 black pixels is considered empty) */
            if (column_sum < 2) {

                /* If capture mode was already enabled, this must be the end of a segment */
                if (capture_segment) {

                    /* Decode the segment signature */
                    size_t segment_value = solver_segment_value(segment_signature);
                    if (segment_value == 0) {
                        solver->solved_value = 0;
                        fprintf(stderr, "Unrecognized segment signature: %zu\n", segment_signature);
                        break;

                    /* Append the segment's digit to the solved value */
                    } else {
                        solver->solved_value *= 10;
                        solver->solved_value += segment_value;

                        /* Reset local state for next segment */
                        capture_segment = false;
                        column_index_multiplier = 0;
                        segment_signature = 0;
                    }
                }

            /* Non-empty column; enable capture mode */
            } else if (!capture_segment) {
                capture_segment = true;
            }

            /* Increment the segment signature with the appropriate multiplier */
            if (capture_segment) {
                segment_signature += column_sum * ++column_index_multiplier;
            }
        }

        /* Success */
        if (solver->solved_value != 0) {
            retval = true;
        }
    }
    return retval;
}

/* Destroys the solver context and frees memory */
void solver_terminate(struct solver_context* solver) {

    /* Free memory allocated for MagickCore objects */
    if (solver->image_view != NULL) {
        DestroyCacheView(solver->image_view);
    }
    if (solver->image != NULL) {
        DestroyImage(solver->image);
    }
    DestroyImageInfo(solver->image_info);
    DestroyExceptionInfo(solver->exception);

    /* Terminate MagickCore */
    MagickCoreTerminus();
}
