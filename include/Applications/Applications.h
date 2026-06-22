#include "include/Applications/SurfaceModel/polyModel.h"
#include "include/Applications/Interpolate/Interp_kriging_smp.h"
#include "include/Applications/Interpolate/Interp_manager.h"
// /* debug end */
namespace iono
{
    // extern t_VariogramM spherical_fit(const size_t n, double *h, double *gamma);
    extern double spherical_variogram(double nugget, double sill, double range, double h);
    t_VariogramM spherical_fit(const size_t n, double *h, double *gamma);
    extern t_VariogramM exp_fit(double *xdata, double *ydata, int n_data);
}
