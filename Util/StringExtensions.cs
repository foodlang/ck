using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace ck.Util;
public static class StringExtensions
{
    /// <summary>
    /// Converts special characters to escape sequences.
    /// </summary>
    public static string Escapize(this string str)
    {
        var result = new StringBuilder(str.Length);

        for (var i = 0; i < str.Length; i++)
        {
            switch (str[i])
            {
            case '\'': result.Append("\\'"); break;
            case '\"': result.Append("\\\""); break;
            case '`': result.Append("\\`"); break; // nasm-only
            case '\\': result.Append("\\\\"); break;
            case '?': result.Append("\\?"); break;
            case '\a': result.Append("\\a"); break;
            case '\b': result.Append("\\b"); break;
            case '\t': result.Append("\\t"); break;
            case '\v': result.Append("\\v"); break;
            case '\f': result.Append("\\f"); break;
            case '\r': result.Append("\\r"); break;
            case '\n': result.Append("\\n"); break;
            case (char)27: result.Append("\\e"); break;

            case >= ' ' and <= '~':
                result.Append(str[i]);
                break;

            default:
                result.Append($"\\x{Convert.ToString((byte)str[i], 16)}"); // when we dunno
                break;
            }
        }

        return result.ToString();
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static void ApplyMask(this Span<char> s, ReadOnlySpan<char> mask, char replace)
    {
        for (var i = 0; i < s.Length; i++)
            if (!mask.Contains(s[i]))
                s[i] = replace;
    }
}
