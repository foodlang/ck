namespace ck.Util;
public static class ArrayExtensions
{
    public static T[] RemoveAt<T>(this T[] src, int index)
    {
        var @new = new T[src.Length - 1];
        if (index > 0)
            Array.Copy(src, 0, @new, 0, index);

        if (index < src.Length - 1)
            Array.Copy(src, index + 1, @new, index, src.Length - index - 1);
        return @new;
    }

    public static T[] RemoveWhere<T>(this T[] src, Func<T, bool> predicate)
    {
        const int Mask_Unit_Size = sizeof(byte);

        Span<byte> delete_mask = stackalloc byte[(src.Length + Mask_Unit_Size - 1) / Mask_Unit_Size];

        var size_delta = 0;
        for (var i = 0; i < src.Length; i++)
            if (predicate(src[i]))
            {
                delete_mask[i / Mask_Unit_Size] |= (byte)(1 << (i % Mask_Unit_Size));
                size_delta++;
            }

        var new_size = src.Length - size_delta;
        if (new_size == src.Length)
            return src;
        if (new_size == 0)
            return Array.Empty<T>();

        var @new = new T[new_size];
        var @new_index = 0;
        for (var i = 0; i < src.Length; i++)
        {
            if ((delete_mask[i / Mask_Unit_Size] & (byte)(1 << (i % Mask_Unit_Size))) != 0)
                @new[new_index++] = src[i];
        }
        return @new;
    }
}
