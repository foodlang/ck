using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace ck.Util;

public static class FloatExtensions
{
    public static uint F32Bits(this float x)
    {
        unsafe
        {
            return *(uint*)&x;
        }
    }

    public static float I32_F32Bits(this uint x)
    {
        unsafe
        {
            return *(float*)&x;
        }
    }

    public static ulong F64Bits(this double x)
    {
        unsafe
        {
            return *(ulong*)&x;
        }
    }

    public static double I64_F64Bits(this ulong x)
    {
        unsafe
        {
            return *(double*)&x;
        }
    }

    public static unsafe float I32_F32Bits(this ulong x)
    {
        if (BitConverter.IsLittleEndian)
            return *(float*)&x;
        else
            return *((float*)&x + 1);
    }
}
