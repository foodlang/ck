using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.Util;

/// <summary>
/// Extensions for <see cref="bool"/>.
/// </summary>
public static class BoolExtensions
{
    /// <summary>
    /// Converts a boolean to an integer.
    /// </summary>
    /// <param name="value">The boolean.</param>
    /// <returns>1 or 0.</returns>
    public static int Int(this bool value)
        => value ? 1 : 0;
}
