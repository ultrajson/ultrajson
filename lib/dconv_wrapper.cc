#include "double-conversion.h"

#define DCONV_DECIMAL_IN_SHORTEST_LOW -4
#define DCONV_DECIMAL_IN_SHORTEST_HIGH 16

enum dconv_d2s_flags {
    DCONV_D2S_NO_FLAGS = 0,
    DCONV_D2S_EMIT_POSITIVE_EXPONENT_SIGN = 1,
    DCONV_D2S_EMIT_TRAILING_DECIMAL_POINT = 2,
    DCONV_D2S_EMIT_TRAILING_ZERO_AFTER_POINT = 4,
    DCONV_D2S_UNIQUE_ZERO = 8
  };

enum dconv_s2d_flags
{
    DCONV_S2D_NO_FLAGS = 0,
    DCONV_S2D_ALLOW_HEX = 1,
    DCONV_S2D_ALLOW_OCTALS = 2,
    DCONV_S2D_ALLOW_TRAILING_JUNK = 4,
    DCONV_S2D_ALLOW_LEADING_SPACES = 8,
    DCONV_S2D_ALLOW_TRAILING_SPACES = 16,
    DCONV_S2D_ALLOW_SPACES_AFTER_SIGN = 32
  };

namespace double_conversion
{
  extern "C"
  {
    int dconv_d2s(double value, char* buf, int buflen, int* strlength, int allow_nan)
    {
    DoubleToStringConverter* d2s;
		if(allow_nan){
      static DoubleToStringConverter d2s_w_nan(DCONV_D2S_EMIT_TRAILING_DECIMAL_POINT | DCONV_D2S_EMIT_TRAILING_ZERO_AFTER_POINT | DCONV_D2S_EMIT_POSITIVE_EXPONENT_SIGN,
                    "Infinity", "NaN", 'e', DCONV_DECIMAL_IN_SHORTEST_LOW, DCONV_DECIMAL_IN_SHORTEST_HIGH, 0, 0);
      d2s = &d2s_w_nan;
    }else{
      static DoubleToStringConverter d2s_wo_nan(DCONV_D2S_EMIT_TRAILING_DECIMAL_POINT | DCONV_D2S_EMIT_TRAILING_ZERO_AFTER_POINT | DCONV_D2S_EMIT_POSITIVE_EXPONENT_SIGN,
                 NULL, NULL, 'e', DCONV_DECIMAL_IN_SHORTEST_LOW, DCONV_DECIMAL_IN_SHORTEST_HIGH, 0, 0);
      d2s = &d2s_wo_nan;
    }

		
        StringBuilder sb(buf, buflen);
        int success =  static_cast<int>(d2s->ToShortest(value, &sb));
        *strlength = success ? sb.position() : -1;
        return success;
    }

    double dconv_s2d(const char* buffer, int length, int* processed_characters_count)
    {
        static StringToDoubleConverter s2d(DCONV_S2D_ALLOW_TRAILING_JUNK, 0.0, 0.0, "Infinity", "NaN");
        return s2d.StringToDouble(buffer, length, processed_characters_count);
    }
  }
}
