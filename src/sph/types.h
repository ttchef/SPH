
#pragma once

//
// NOTE: This file is used to make opaque pointers for strucs public
//

typedef struct app    app;
typedef struct window window;

typedef enum
{
    GRADIENT_NONE,
    GRADIENT_HORIZOTNAL = 1,
    GRADIENT_VERTICAL   = 2,
} gradient_type;
