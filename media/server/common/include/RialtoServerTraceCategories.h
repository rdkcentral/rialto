//
//  RialtoServerTraceCategories.h
//
//  Copyright © 2023 Sky UK. All rights reserved.
//
#ifndef RIALTOSERVERTRACECATEGORIES_H
#define RIALTOSERVERTRACECATEGORIES_H


#include <perfetto.h>

// the set of track event categories that the example is using.
PERFETTO_DEFINE_CATEGORIES(
    perfetto::Category("GstMediaPipeline")
        .SetDescription("Gst Media Pipeline")
);


#endif // RIALTOSERVERTRACECATEGORIES_H
