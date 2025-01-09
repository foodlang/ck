using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.Ssa;

/// <summary>
/// The type of a SSA register.
/// </summary>
public enum SsaType
{
    None = 0,
    B8,
    B16,
    B32,
    B64,

    F32,
    F64,

    Ptr,
}
