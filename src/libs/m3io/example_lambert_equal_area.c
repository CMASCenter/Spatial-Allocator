/*
example_lambert_equal_area.c - Demonstrate createLambertEqualAreaM3IOFile().
cc -I. -o example_lambert_equal_area example_lambert_equal_area.c M3IO.c \
   libnetcdf.a
./example_lambert_equal_area
ncdump example_lambert_equal_area.ncf
*/

#include <M3IO.h> /* For Name, Line, createLambertEqualAreaM3IOFile(). */


int main( void ) {
  const char* const fileName = "example_lambert_equal_area.ncf";
  enum {
    VARIABLES = 2, TIMESTEPS = 2, LAYERS = 3, ROWS = 5, COLUMNS = 4,
    YYYYDDD = 2002113
  };
  const double cellSize         = 2000.0;
  const double westEdge         = 1578000.0;
  const double southEdge        = -1270000.0;
  const double longitude0       = -100.0;
  const double latitude0        = 40.0;
  const double centralLongitude = -100.0;
  const double centralLatitude  = 60.0;
  const double topPressure = 10000.0;
  const double sigmaLevels[ LAYERS + 1 ] = { 1.0, 0.995, 0.99, 0.985 };
  const Name variableNames[ VARIABLES ] = { "O3", "NO" };
  const Name variableUnits[ VARIABLES ] = { "ppmV", "ppmV" };
  const Line variableDescriptions[ VARIABLES ] = {
    "Ozone concentration",
    "Nitrogen-Oxide concentration"
  };
  const Name gridName = { "M_02_99BRACE" };
  const Description description =
    "Concentration file output from CMAQ model dyn alloc version CTM";

  const float dataO3[ TIMESTEPS * LAYERS * ROWS * COLUMNS ] = {
    0.03664103, 0.03672275, 0.03678665, 0.03682948,
    0.03651726, 0.03648462, 0.03639814, 0.03637281,
    0.03616524, 0.03602019, 0.03601136, 0.03608273,
    0.03577007, 0.03582532, 0.03591563, 0.03603813,
    0.03574514, 0.0357785, 0.0358368, 0.03597659,
    0.0366466, 0.03672829, 0.03679417, 0.03683609,
    0.03652334, 0.03649161, 0.03640601, 0.03638419,
    0.03617217, 0.0360271, 0.03602079, 0.03609426,
    0.03577468, 0.03583254, 0.03592617, 0.03605448,
    0.03575096, 0.03578655, 0.03585205, 0.03599941,
    0.03665371, 0.0367355, 0.03680277, 0.03684599,
    0.03653148, 0.03650082, 0.03641789, 0.03639891,
    0.03617895, 0.03603633, 0.03603363, 0.03611036,
    0.03578287, 0.03584192, 0.03594097, 0.03607785,
    0.0357573, 0.03579806, 0.03587398, 0.03603246,
    0.03606629, 0.03611911, 0.0361395, 0.03612331,
    0.03595442, 0.03591379, 0.03581693, 0.03581721,
    0.03573601, 0.03564837, 0.03565824, 0.0357287,
    0.03550548, 0.03554794, 0.03560167, 0.03565291,
    0.03547987, 0.03549325, 0.03549294, 0.03550721,
    0.03607113, 0.03612455, 0.03614542, 0.03613177,
    0.03595964, 0.03591971, 0.03582362, 0.03582685,
    0.03574118, 0.03565389, 0.03566666, 0.03573858,
    0.03551019, 0.03555351, 0.03561042, 0.03566507,
    0.03548468, 0.03549965, 0.0355034, 0.03552223,
    0.03607704, 0.03613118, 0.03615335, 0.03614324,
    0.03596605, 0.03592739, 0.03583285, 0.03583996,
    0.03574741, 0.03566163, 0.03567756, 0.03575243,
    0.03551595, 0.03556076, 0.03562266, 0.03568231,
    0.03549056, 0.03550811, 0.03551827, 0.03554418
  };

  const float dataNO[ TIMESTEPS * LAYERS * ROWS * COLUMNS ] = {
    1.043758e-06, 1.051559e-06, 1.061139e-06, 1.068576e-06,
    1.027751e-06, 1.019895e-06, 1.01026e-06, 1.006717e-06,
    9.825767e-07, 9.61256e-07, 9.588275e-07, 9.684978e-07,
    9.287292e-07, 9.336006e-07, 9.448314e-07, 9.629399e-07,
    9.265194e-07, 9.302212e-07, 9.393401e-07, 9.560709e-07,
    1.058448e-06, 1.066191e-06, 1.076004e-06, 1.083622e-06,
    1.042294e-06, 1.034236e-06, 1.024615e-06, 1.021253e-06,
    9.965024e-07, 9.747858e-07, 9.725138e-07, 9.828278e-07,
    9.417149e-07, 9.466837e-07, 9.5857e-07, 9.77505e-07,
    9.395238e-07, 9.433754e-07, 9.533033e-07, 9.7132e-07,
    1.073932e-06, 1.08176e-06, 1.091872e-06, 1.09973e-06,
    1.057646e-06, 1.04958e-06, 1.040092e-06, 1.03701e-06,
    1.011169e-06, 9.893184e-07, 9.872891e-07, 9.985858e-07,
    9.554602e-07, 9.607512e-07, 9.73562e-07, 9.93633e-07,
    9.532635e-07, 9.575817e-07, 9.687128e-07, 9.885109e-07,
    1.766733e-09, 3.183934e-10, 6.264217e-11, 5.284733e-11,
    1.870396e-09, 3.53175e-10, 5.999193e-11, 4.828008e-11,
    1.912553e-09, 3.659315e-10, 5.869511e-11, 4.726193e-11,
    1.80713e-09, 3.606381e-10, 5.826457e-11, 4.635787e-11,
    1.848069e-09, 3.75745e-10, 5.78058e-11, 4.467788e-11,
    1.804707e-09, 3.24388e-10, 6.269493e-11, 5.274105e-11,
    1.910647e-09, 3.60099e-10, 6.013405e-11, 4.820302e-11,
    1.954198e-09, 3.732762e-10, 5.88823e-11, 4.720314e-11,
    1.846043e-09, 3.678995e-10, 5.847579e-11, 4.633479e-11,
    1.887765e-09, 3.83449e-10, 5.806165e-11, 4.470051e-11,
    1.84311e-09, 3.306617e-10, 6.261999e-11, 5.252037e-11,
    1.951637e-09, 3.673823e-10, 6.017508e-11, 4.802557e-11,
    1.996114e-09, 3.810724e-10, 5.89867e-11, 4.705004e-11,
    1.885184e-09, 3.755562e-10, 5.861111e-11, 4.623078e-11,
    1.928493e-09, 3.915679e-10, 5.825505e-11, 4.466725e-11
  };

  const int file =
    createLambertEqualAreaM3IOFile(
      fileName, VARIABLES, TIMESTEPS, LAYERS, ROWS, COLUMNS, YYYYDDD,
      cellSize, westEdge, southEdge,
      longitude0, latitude0,
      centralLongitude, centralLatitude,
      topPressure, sigmaLevels,
      variableNames, variableUnits, variableDescriptions,
      gridName, description );

  int ok = file != -1;

  if ( file != -1 ) {
    ok = writeM3IOData( file, "O3", dataO3 );
    ok = ok && writeM3IOData( file, "NO", dataNO );
    ok = closeM3IOFile( file ) && ok;
  }

  return ! ok;
}



