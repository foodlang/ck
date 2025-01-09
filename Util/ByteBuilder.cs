using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace ck.Util;

/// <summary>
/// Buffered byte array used for writing byte output. Allocates
/// by page.
/// </summary>
public sealed class ByteBuilder
{
    /// <summary>
    /// The size of page allocations.
    /// </summary>
    public readonly int PageSize;

    /// <summary>
    /// The position in the buffer.
    /// </summary>
    public int Position;

    /// <summary>
    /// The current size of the buffer.
    /// </summary>
    public int CurrentSize;

    /// <summary>
    /// The internal byte buffer.
    /// </summary>
    private unsafe byte* _buffer;

    /// <summary>
    /// Allocates a new byte builder with a given page size.
    /// </summary>
    public ByteBuilder(int page_size)
    {
        unsafe
        {
            PageSize = page_size;
            Position = 0;
            CurrentSize = PageSize;
            _buffer = (byte*)Marshal.AllocHGlobal(page_size);
        }
    }

    /// <summary>
    /// Increases the buffer size if possible.
    /// </summary>
    /// <param name="minimum_bytes">The amount of bytes that we *need*.</param>
    private unsafe void EnforceBufferSize(int minimum_bytes)
    {
        var total = Position + minimum_bytes;
        if (CurrentSize < total)
        {
            var excess = CurrentSize - total;
            var pages_required = (int)(Math.Ceiling(excess / (float)PageSize));
            CurrentSize += pages_required * PageSize;
            _buffer = (byte*)Marshal.ReAllocHGlobal((nint)_buffer, CurrentSize);
        }
    }

    /// <summary>
    /// Writes a u8 value.
    /// </summary>
    public void WriteU8(byte b)
    {
        unsafe
        {
            EnforceBufferSize(1);
            _buffer[Position++] = b;
        }
    }

    /// <summary>
    /// Writes a u16 value.
    /// </summary>
    public void WriteU16(ushort s)
    {
        unsafe
        {
            EnforceBufferSize(2);
            if (BitConverter.IsLittleEndian)
            {
                _buffer[Position++] = (byte)(s & 0xFF);
                _buffer[Position++] = (byte)((s >> 8) & 0xFF);
            }
            else
            {
                _buffer[Position++] = (byte)((s >> 8) & 0xFF);
                _buffer[Position++] = (byte)(s & 0xFF);
            }
        }
    }

    /// <summary>
    /// Writes a u32 value.
    /// </summary>
    public void WriteU32(uint u)
    {
        unsafe
        {
            EnforceBufferSize(2);
            if (BitConverter.IsLittleEndian)
            {
                _buffer[Position++] = (byte)(u & 0xFF);
                _buffer[Position++] = (byte)((u >> 8) & 0xFF);
                _buffer[Position++] = (byte)((u >> 16) & 0xFF);
                _buffer[Position++] = (byte)((u >> 24) & 0xFF);
            }
            else
            {
                _buffer[Position++] = (byte)((u >> 24) & 0xFF);
                _buffer[Position++] = (byte)((u >> 16) & 0xFF);
                _buffer[Position++] = (byte)((u >> 8) & 0xFF);
                _buffer[Position++] = (byte)(u & 0xFF);
            }
        }
    }

    /// <summary>
    /// Writes a u64 value.
    /// </summary>
    public void WriteU64(ulong l)
    {
        unsafe
        {
            EnforceBufferSize(2);
            if (BitConverter.IsLittleEndian)
            {
                _buffer[Position++] = (byte)(l & 0xFF);
                _buffer[Position++] = (byte)((l >> 8) & 0xFF);
                _buffer[Position++] = (byte)((l >> 16) & 0xFF);
                _buffer[Position++] = (byte)((l >> 24) & 0xFF);
                _buffer[Position++] = (byte)((l >> 32) & 0xFF);
                _buffer[Position++] = (byte)((l >> 40) & 0xFF);
                _buffer[Position++] = (byte)((l >> 48) & 0xFF);
                _buffer[Position++] = (byte)((l >> 56) & 0xFF);
            }
            else
            {
                _buffer[Position++] = (byte)((l >> 56) & 0xFF);
                _buffer[Position++] = (byte)((l >> 48) & 0xFF);
                _buffer[Position++] = (byte)((l >> 40) & 0xFF);
                _buffer[Position++] = (byte)((l >> 32) & 0xFF);
                _buffer[Position++] = (byte)((l >> 24) & 0xFF);
                _buffer[Position++] = (byte)((l >> 16) & 0xFF);
                _buffer[Position++] = (byte)((l >> 8) & 0xFF);
                _buffer[Position++] = (byte)(l & 0xFF);
            }
        }
    }

    /// <summary>
    /// Writes a f16 value.
    /// </summary>
    public void WriteF16(Half h)
    {
        unsafe
        {
            WriteU16(*(ushort*)&h);
        }
    }

    /// <summary>
    /// Writes a f32 value.
    /// </summary>
    public void WriteF32(float f)
    {
        unsafe
        {
            WriteU32(*(uint*)&f);
        }
    }

    /// <summary>
    /// Writes a f64 value.
    /// </summary>
    public void WriteF64(double d)
    {
        unsafe
        {
            WriteF64(*(ulong*)&d);
        }
    }

    /// <summary>
    /// Writes a string.
    /// </summary>
    public void WriteString(string str)
    {
        var strlen = str.Length;
        EnforceBufferSize(strlen + 1);
        unsafe
        {
            fixed (char *cstr = str)
            {
                while (strlen > 0)
                    _buffer[Position++] = (byte)cstr[strlen--];
            }
            _buffer[Position++] = 0; // null terminator
        }
    }

    unsafe ~ByteBuilder()
    {
        Marshal.FreeHGlobal((nint)_buffer);
    }
}
