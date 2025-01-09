using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.XIL;

/// <summary>
/// The storage of a virtual X-register
/// </summary>
public enum XRegStorage : byte
{
    /// <summary>
    /// Automatic local storage
    /// </summary>
    Local,

    /// <summary>
    /// Local/RAM storage
    /// </summary>
    LocalMustPhysical,

    /// <summary>
    /// Fast storage (used for temporaries)
    /// </summary>
    Fast,

    /// <summary>
    /// Function parameter
    /// </summary>
    Param,

    /// <summary>
    /// Static storage
    /// </summary>
    Static,
}

/// <summary>
/// An X-register
/// </summary>
public struct XReg
{
    /// <summary>
    /// Internal integer (-1=noreg)
    /// </summary>
    public int R;

    /// <summary>
    /// The storage of the register.
    /// </summary>
    public XRegStorage Storage;

    /// <summary>
    /// Whether the address is a local offset in the function.
    /// </summary>
    public bool LocalOffset;

    public XReg(int r, XRegStorage storage)
    {
        R = r;
        Storage = storage;
    }

    public static XReg NoReg() => new(-1, XRegStorage.Fast);
}
