#pragma once

// geometry::Ray reuses hr::Ray (struct Ray { Vec3 origin, direction; Vec3 At(float) const; })
// defined in core/math.h. Do not redefine a separate Ray type here.
#include "../core/math.h"
