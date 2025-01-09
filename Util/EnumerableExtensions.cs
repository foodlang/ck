using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.Util;

/// <summary>
/// Extensions for <see cref="IEnumerable{T}"/>.
/// </summary>
public static class EnumerableExtensions
{
    /// <summary>
    /// Matches an <see cref="IEnumerable{T}"/> against another.
    /// </summary>
    /// <returns>Returns the amount of matching elements.</returns>
    public static int Match<T>(this IEnumerable<T> enumerable, IEnumerable<T> other)
        => enumerable.Intersect(other).Count();

    /// <summary>
    /// Checks if all entries are matching.
    /// </summary>
    public static bool FullMatch<T>(this IEnumerable<T> enumerable, IEnumerable<T> against)
        => Match(enumerable, against) == against.Count();
}
