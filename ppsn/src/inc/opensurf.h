// This code provides standalone definitions of the OpenSURF descriptor and
// examples on how to load and compare descriptors. To obtain the OpenSURF
// toolkit, which offers additional functionality, refer to the following
// website: www.chrisevansdev.com/computer-vision-opensurf.html

#include <stddef.h>
#include <stdio.h>
#include <float.h>

#ifndef __OPENSUERF_H__
#define __OPENSUERF_H__

#define OPENSURF_FEATURECOUNT 64

class OpenSurfInterestPoint {

public:

    // Constructor
    OpenSurfInterestPoint() : orientation(0) {};
    // Destructor
    ~OpenSurfInterestPoint() {};
    // Coordinates of the detected interest point
    float x, y;
    // Detected scale
    float scale;
    // Orientation measured anti-clockwise from +ve x-axis
    float orientation;
    // Sign of laplacian for fast matching purposes
    int laplacian;
    // Vector of descriptor components
    float descriptor[OPENSURF_FEATURECOUNT];
};

bool OpenSurf_LoadDescriptor(const char *fname, OpenSurfInterestPoint *&points, int &ipoints)
{
    if (fname == NULL)
    {
        fprintf(stderr, "an empty filename was provided\n");
        return false;
    }
    // open the file
    FILE *file = fopen(fname, "rb");
    if (file == NULL)
    {
        fprintf(stderr, "could not open %s\n", fname);
        return false;
    }
    // read the number of points
    if (fread(&ipoints, sizeof(int), 1, file) != 1)
        goto failure;
    // read the point data
    points = NULL;
    if (ipoints > 0)
    {
        points = new OpenSurfInterestPoint[ipoints];
        for (int i = 0; i < ipoints; i++)
        {
            if (fread(&points[i].x, sizeof(float), 1, file) != 1 ||
                fread(&points[i].y, sizeof(float), 1, file) != 1 ||
                fread(&points[i].scale, sizeof(float), 1, file) != 1 ||
                fread(&points[i].orientation, sizeof(float), 1, file) != 1 ||
                fread(&points[i].laplacian, sizeof(int), 1, file) != 1 ||
                fread(points[i].descriptor, sizeof(float), OPENSURF_FEATURECOUNT, file) != OPENSURF_FEATURECOUNT)
                goto failure;
        }
    }
    // close the file
    fclose(file);
    return true;

failure:
    printf("could not read from file : %s \n", fname);
    delete[] points;
    fclose(file);
    return false;
}


#endif