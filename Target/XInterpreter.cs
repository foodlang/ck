using ck.Diagnostics;
using ck.Util;
using ck.XIL;
using System.Diagnostics;
using System.Drawing;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using static ck.Target.XInterpreter;

namespace ck.Target;

public sealed class XInterpreter : XTarget<Empty, CallFrame>
{
    public override string GetName() => "runx";

    public override unsafe void CreateFunctionHeader(XMethod func, ref XTargetState<CallFrame> state)
    {
        

        /*state.Frame.RegisterTable = new ulong[];

        state.Frame.Blocks = new void*[func.MethodBlocks.Count];
        foreach (var blk in func.MethodBlocks)
            state.Frame.Blocks[blk.Value.Index] = (void*)Marshal.AllocHGlobal((int)blk.Value.Size);

        for (var i = 0; i < state.Frame.Params.Length; i++)
            state.Frame.RegisterTable[i] = state.Frame.Params[i];*/

        state.Frame.StackFrameBase = state.Frame.StackFrameTop;
        state.Frame.StackFrameTop += (uint)func.GetRegisterCount() * sizeof(long);

        // blocks
        foreach (var blk in func.MethodBlocks)
        {
            state.Frame.Blocks[blk.Value.Index] = state.Frame.StackFrameTop;
            state.Frame.StackFrameTop += blk.Value.Size;
        }

        if (sz_StackHostBlock == 0)
        {
            sz_StackHostBlock = state.Frame.StackFrameTop - Stack_Start;
            p_StackHostBlock = NativeMemory.Alloc((nuint)sz_StackHostBlock);
        }
        else if (state.Frame.StackFrameTop - Stack_Start > sz_StackHostBlock)
        {
            // i know there are better scaling factors for size efficiency,
            // but we care more about the speed of access.
            sz_StackHostBlock <<= 2;
            p_StackHostBlock = NativeMemory.Realloc(p_StackHostBlock, (nuint)sz_StackHostBlock);
        }

        fixed (ulong* p_Params = state.Frame.Params)
            NativeMemory.Copy(p_Params, (byte*)p_StackHostBlock + state.Frame.StackFrameBase, (uint)state.Frame.Params.Length * sizeof(ulong));
    }

    public override unsafe void CreateFunctionFooter(XMethod func, ref XTargetState<CallFrame> state)
    {
        //foreach (var blk in state.Frame.Blocks)
        //    Marshal.FreeHGlobal((nint)blk);
    }

    public override Empty ProduceImage(XModule module) => new Empty();

    public unsafe struct CallFrame
    {
        public CallFrame()
        {
            Params = [];
            Blocks = [];
            StackFrameBase = 0;
            StackFrameTop = 0;
            ReturnRegister = -1;
        }

        /// <summary>
        /// The parameters of the functions. This is copied by the function header
        /// into registers.
        /// </summary>
        public ulong[] Params;

        /// <summary>
        /// The blocks of the function, easily represented as a list of pointers
        /// indexed by block ID.
        /// </summary>
        public ulong[] Blocks;

        /// <summary>
        /// The current start of the stack.
        /// </summary>
        public ulong StackFrameBase;

        /// <summary>
        /// The current end of the stack.
        /// </summary>
        public ulong StackFrameTop;

        /// <summary>
        /// -1 -> no return expected, ignore last_return_value
        /// </summary>
        public int ReturnRegister;
    }

    public const ulong Stack_Start = 0x0000_2001_0000;
    public const ulong Stack_End   = 0x00FF_FFFF_FFFF;

    public const ulong VMem_ProgramStart = 0x10000;
    public ulong VMem_ProgramEnd = VMem_ProgramStart;
    public ulong VMem_ProgramSize => VMem_ProgramEnd - VMem_ProgramStart;

    public unsafe void* p_StackHostBlock = null;
    public ulong sz_StackHostBlock = 0;

    /**
     * 
     * Virtual memory map of the X interpreter:
     * 
     * 0x0000000000000->0x00000000FFFF: nothing
     * 0x0000000010000->0x00002000FFFF: reserved for program code
     * 0x0000200010000->0x00FFFFFFFFFF: stack
     * (the rest is provided by pages which must be requested by the user code.
     *  this is usually done by a memory management system such as malloc()/free()
     *  or arena allocators.)
     */

    /// <summary>
    /// Virtual MMU that maps a given comptime address to a compiler address.
    /// </summary>
    /// <param name="vaddr"></param>
    /// <returns></returns>
    public unsafe ulong MapVirtualAddress(XMethod func, CallFrame frame, ulong vaddr)
    {
        int error = 0;

        // no object under 0x10000
        if (vaddr <= 0xFFFF)
        {
            error = 1; // low pointer
            goto LInvalidAddr;
        }

        // program code!
        if (vaddr <= VMem_ProgramEnd)
        {
            var code_addr = vaddr - VMem_ProgramStart;

            if (code_addr > (ulong)func.GetInsnCount())
            {
                error = 3;
                goto LInvalidAddr;
            }

            return code_addr;
        }

        // we are looking at stack memory
        if (vaddr <= Stack_End)
        {
            var stack_relative = vaddr - Stack_Start;
            if (stack_relative > frame.StackFrameTop)
            {
                error = 2; // out of stack bounds
                goto LInvalidAddr;
            }

            return (ulong)p_StackHostBlock + stack_relative;
        }

// otherwise, check virtual memory pages
;

LInvalidAddr:
        if (error == 0)
            Diagnostic.Throw(null, 0, DiagnosticSeverity.Error, "", $"invalid address 0x{vaddr:X}");
        else if (error == 1)
            Diagnostic.Throw(null, 0, DiagnosticSeverity.Error, "", $"invalid address 0x{vaddr:X} (pointer is too low, most likely an object size)");
        else if (error == 2)
            Diagnostic.Throw(null, 0, DiagnosticSeverity.Error, "", $"invalid address 0x{vaddr:X} (pointer is within stack range, yet out of bounds)");
        else if (error == 3)
            Diagnostic.Throw(null, 0, DiagnosticSeverity.Error, "", $"invalid address 0x{vaddr:X} (pointer is in program range, but is out of local function)");

        return 0;
    }

    public unsafe void ProcessF64Insn(XMethod func, ref XTargetState<CallFrame> state, ref int ip, ulong* p_RegisterTable)
    {
        var insn = func.GetInsn(ip);
        var frame = state.Frame;
        var insn_type = insn.Type;

        switch (insn.Op)
        {

        // CONST ARITHM OPERATIONS

        case XOps.Increment:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rd.R].I64_F64Bits() + ((ulong)insn.Kx[0]).I64_F64Bits()).F64Bits();
            break;

        case XOps.Decrement:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rd.R].I64_F64Bits() - ((ulong)insn.Kx[0]).I64_F64Bits()).F64Bits();
            break;

        case XOps.MulConst:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rd.R].I64_F64Bits() * ((ulong)insn.Kx[0]).I64_F64Bits()).F64Bits();
            break;

        case XOps.DivConst:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rd.R].I64_F64Bits() / ((ulong)insn.Kx[0]).I64_F64Bits()).F64Bits();
            break;

        // COMPARISON

        case XOps.Equal:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rsx[0].R].I64_F64Bits() == p_RegisterTable[insn.Rsx[1].R].I64_F64Bits()) ? 1UL : 0;
            break;

        case XOps.NotEqual:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rsx[0].R].I64_F64Bits() != p_RegisterTable[insn.Rsx[1].R].I64_F64Bits()) ? 1UL : 0;
            break;

        case XOps.Lower:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rsx[0].R].I64_F64Bits() < p_RegisterTable[insn.Rsx[1].R].I64_F64Bits()) ? 1UL : 0;
            break;

        case XOps.LowerEqual:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rsx[0].R].I64_F64Bits() <= p_RegisterTable[insn.Rsx[1].R].I64_F64Bits()) ? 1UL : 0;
            break;

        case XOps.Greater:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rsx[0].R].I64_F64Bits() > p_RegisterTable[insn.Rsx[1].R].I64_F64Bits()) ? 1UL : 0;
            break;

        case XOps.GreaterEqual:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rsx[0].R].I64_F64Bits() >= p_RegisterTable[insn.Rsx[1].R].I64_F64Bits()) ? 1UL : 0;
            break;

        // ARITHMETIC

        case XOps.UnaryMinus:
            p_RegisterTable[insn.Rd.R] = (-p_RegisterTable[insn.Rsx[0].R].I64_F64Bits()).F64Bits();
            break;

        case XOps.Add:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rsx[0].R].I64_F64Bits() + p_RegisterTable[insn.Rsx[1].R].I64_F64Bits()).F64Bits();
            break;

        case XOps.Sub:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rsx[0].R].I64_F64Bits() - p_RegisterTable[insn.Rsx[1].R].I64_F64Bits()).F64Bits();
            break;

        case XOps.Mul:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rsx[0].R].I64_F64Bits() * p_RegisterTable[insn.Rsx[1].R].I64_F64Bits()).F64Bits();
            break;

        case XOps.Div:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rsx[0].R].I64_F64Bits() / p_RegisterTable[insn.Rsx[1].R].I64_F64Bits()).F64Bits();
            break;

        case XOps.Rem:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rsx[0].R].I64_F64Bits() % p_RegisterTable[insn.Rsx[1].R].I64_F64Bits()).F64Bits();
            break;

        }
    }

    public unsafe void ProcessF32Insn(XMethod func, ref XTargetState<CallFrame> state, ref int ip, ulong* p_RegisterTable)
    {
        var insn = func.GetInsn(ip);
        var frame = state.Frame;
        var insn_type = insn.Type;

        switch (insn.Op)
        {

        // CONST ARITHM OPERATIONS

        case XOps.Increment:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rd.R].I32_F32Bits() + (float)((ulong)insn.Kx[0]).I64_F64Bits()).F32Bits();
            break;

        case XOps.Decrement:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rd.R].I32_F32Bits() - (float)((ulong)insn.Kx[0]).I64_F64Bits()).F32Bits();
            break;

        case XOps.MulConst:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rd.R].I32_F32Bits() * (float)((ulong)insn.Kx[0]).I64_F64Bits()).F32Bits();
            break;

        case XOps.DivConst:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rd.R].I32_F32Bits() / (float)((ulong)insn.Kx[0]).I64_F64Bits()).F32Bits();
            break;

        // COMPARISON

        case XOps.Equal:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rd.R].I32_F32Bits() == ((uint)p_RegisterTable[insn.Rd.R]).I32_F32Bits()) ? 1UL : 0;
            break;

        case XOps.NotEqual:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rd.R].I32_F32Bits() != ((uint)p_RegisterTable[insn.Rd.R]).I32_F32Bits()) ? 1UL : 0;
            break;

        case XOps.Lower:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rd.R].I32_F32Bits() < ((uint)p_RegisterTable[insn.Rd.R]).I32_F32Bits()) ? 1UL : 0;
            break;

        case XOps.LowerEqual:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rd.R].I32_F32Bits() <= ((uint)p_RegisterTable[insn.Rd.R]).I32_F32Bits()) ? 1UL : 0;
            break;

        case XOps.Greater:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rd.R].I32_F32Bits() > ((uint)p_RegisterTable[insn.Rd.R]).I32_F32Bits()) ? 1UL : 0;
            break;

        case XOps.GreaterEqual:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rd.R].I32_F32Bits() >= ((uint)p_RegisterTable[insn.Rd.R]).I32_F32Bits()) ? 1UL : 0;
            break;

        // ARITHMETIC

        case XOps.UnaryMinus:
            p_RegisterTable[insn.Rd.R] = (-p_RegisterTable[insn.Rd.R].I32_F32Bits()).F32Bits();
            break;

        case XOps.Add:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rd.R].I32_F32Bits() + ((uint)p_RegisterTable[insn.Rd.R]).I32_F32Bits()).F32Bits();
            break;

        case XOps.Sub:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rd.R].I32_F32Bits() - ((uint)p_RegisterTable[insn.Rd.R]).I32_F32Bits()).F32Bits();
            break;

        case XOps.Mul:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rd.R].I32_F32Bits() * ((uint)p_RegisterTable[insn.Rd.R]).I32_F32Bits()).F32Bits();
            break;

        case XOps.Div:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rd.R].I32_F32Bits() / ((uint)p_RegisterTable[insn.Rd.R]).I32_F32Bits()).F32Bits();
            break;

        case XOps.Rem:
            p_RegisterTable[insn.Rd.R] = (p_RegisterTable[insn.Rd.R].I32_F32Bits() % ((uint)p_RegisterTable[insn.Rd.R]).I32_F32Bits()).F32Bits();
            break;

        }
    }

    public unsafe void ProcessFloatInsn(XMethod func, ref XTargetState<CallFrame> state, ref int ip, ulong* p_RegisterTable)
    {
        var insn = func.GetInsn(ip);
        var frame = state.Frame;
        var insn_type = insn.Type;

        if (insn_type.Size == 8)
            ProcessF64Insn(func, ref state, ref ip, p_RegisterTable);
        else
            ProcessF32Insn(func, ref state, ref ip, p_RegisterTable);
    }

    public unsafe void ProcessUnsignedInsn(XMethod func, ref XTargetState<CallFrame> state, ref int ip, ulong* p_RegisterTable)
    {
        var insn = func.GetInsn(ip);
        var frame = state.Frame;
        var insn_type = insn.Type;

        switch (insn.Op)
        {

            // CONST ARITHM OPERATIONS

        case XOps.Increment:
            p_RegisterTable[insn.Rd.R] += insn.Kx[0];
            break;

        case XOps.Decrement:
            p_RegisterTable[insn.Rd.R] -= insn.Kx[0];
            break;

        case XOps.MulConst:
            p_RegisterTable[insn.Rd.R] *= insn.Kx[0];
            break;

        case XOps.DivConst:
            p_RegisterTable[insn.Rd.R] /= insn.Kx[0];
            break;

        // COMPARISON

        case XOps.Equal:
            p_RegisterTable[insn.Rd.R] = p_RegisterTable[insn.Rsx[0].R] == p_RegisterTable[insn.Rsx[1].R] ? 1UL : 0;
            break;

        case XOps.NotEqual:
            p_RegisterTable[insn.Rd.R] = p_RegisterTable[insn.Rsx[0].R] != p_RegisterTable[insn.Rsx[1].R] ? 1UL : 0;
            break;

        case XOps.Lower:
            p_RegisterTable[insn.Rd.R] = p_RegisterTable[insn.Rsx[0].R] < p_RegisterTable[insn.Rsx[1].R] ? 1UL : 0;
            break;

        case XOps.LowerEqual:
            p_RegisterTable[insn.Rd.R] = p_RegisterTable[insn.Rsx[0].R] <= p_RegisterTable[insn.Rsx[1].R] ? 1UL : 0;
            break;

        case XOps.Greater:
            p_RegisterTable[insn.Rd.R] = p_RegisterTable[insn.Rsx[0].R] > p_RegisterTable[insn.Rsx[1].R] ? 1UL : 0;
            break;

        case XOps.GreaterEqual:
            p_RegisterTable[insn.Rd.R] = p_RegisterTable[insn.Rsx[0].R] >= p_RegisterTable[insn.Rsx[1].R] ? 1UL : 0;
            break;

            // ARITHMETIC

        case XOps.UnaryMinus:
            p_RegisterTable[insn.Rd.R] = (ulong)-(long)p_RegisterTable[insn.Rsx[0].R];
            break;

        case XOps.Add:
            p_RegisterTable[insn.Rd.R] = p_RegisterTable[insn.Rsx[0].R] + p_RegisterTable[insn.Rsx[1].R];
            break;

        case XOps.Sub:
            p_RegisterTable[insn.Rd.R] = p_RegisterTable[insn.Rsx[0].R] - p_RegisterTable[insn.Rsx[1].R];
            break;

        case XOps.Mul:
            p_RegisterTable[insn.Rd.R] = p_RegisterTable[insn.Rsx[0].R] * p_RegisterTable[insn.Rsx[1].R];
            break;

        case XOps.Div:
            p_RegisterTable[insn.Rd.R] = p_RegisterTable[insn.Rsx[0].R] / p_RegisterTable[insn.Rsx[1].R];
            break;

        case XOps.Rem:
            p_RegisterTable[insn.Rd.R] = p_RegisterTable[insn.Rsx[0].R] % p_RegisterTable[insn.Rsx[1].R];
            break;

        }
    }

    public unsafe void ProcessSignedInsn(XMethod func, ref XTargetState<CallFrame> state, ref int ip, ulong* p_RegisterTable)
    {
        var insn = func.GetInsn(ip);
        var frame = state.Frame;
        var insn_type = insn.Type;

        switch (insn.Op)
        {

        // CONST ARITHM OPERATIONS

        case XOps.Increment:
            p_RegisterTable[insn.Rd.R] = (ulong)((long)p_RegisterTable[insn.Rd.R] + (long)insn.Kx[0]);
            break;

        case XOps.Decrement:
            p_RegisterTable[insn.Rd.R] = (ulong)((long)p_RegisterTable[insn.Rd.R] - (long)insn.Kx[0]);
            break;

        case XOps.MulConst:
            p_RegisterTable[insn.Rd.R] = (ulong)((long)p_RegisterTable[insn.Rd.R] * (long)insn.Kx[0]);
            break;

        case XOps.DivConst:
            p_RegisterTable[insn.Rd.R] = (ulong)((long)p_RegisterTable[insn.Rd.R] / (long)insn.Kx[0]);
            break;

        // COMPARISON

        case XOps.Equal:
            p_RegisterTable[insn.Rd.R] = p_RegisterTable[insn.Rsx[0].R] == p_RegisterTable[insn.Rsx[1].R] ? 1UL : 0;
            break;

        case XOps.NotEqual:
            p_RegisterTable[insn.Rd.R] = p_RegisterTable[insn.Rsx[0].R] != p_RegisterTable[insn.Rsx[1].R] ? 1UL : 0;
            break;

        case XOps.Lower:
            p_RegisterTable[insn.Rd.R] = p_RegisterTable[insn.Rsx[0].R] < p_RegisterTable[insn.Rsx[1].R] ? 1UL : 0;
            break;

        case XOps.LowerEqual:
            p_RegisterTable[insn.Rd.R] = p_RegisterTable[insn.Rsx[0].R] <= p_RegisterTable[insn.Rsx[1].R] ? 1UL : 0;
            break;

        case XOps.Greater:
            p_RegisterTable[insn.Rd.R] = p_RegisterTable[insn.Rsx[0].R] > p_RegisterTable[insn.Rsx[1].R] ? 1UL : 0;
            break;

        case XOps.GreaterEqual:
            p_RegisterTable[insn.Rd.R] = p_RegisterTable[insn.Rsx[0].R] >= p_RegisterTable[insn.Rsx[1].R] ? 1UL : 0;
            break;

        // ARITHMETIC

        case XOps.UnaryMinus:
            p_RegisterTable[insn.Rd.R] = (ulong)-(long)p_RegisterTable[insn.Rsx[0].R];
            break;

        case XOps.Add:
            p_RegisterTable[insn.Rd.R] = (ulong)((long)p_RegisterTable[insn.Rsx[0].R] + (long)p_RegisterTable[insn.Rsx[1].R]);
            break;

        case XOps.Sub:
            p_RegisterTable[insn.Rd.R] = (ulong)((long)p_RegisterTable[insn.Rsx[0].R] - (long)p_RegisterTable[insn.Rsx[1].R]);
            break;

        case XOps.Mul:
            p_RegisterTable[insn.Rd.R] = (ulong)((long)p_RegisterTable[insn.Rsx[0].R] * (long)p_RegisterTable[insn.Rsx[1].R]);
            break;

        case XOps.Div:
            p_RegisterTable[insn.Rd.R] = (ulong)((long)p_RegisterTable[insn.Rsx[0].R] / (long)p_RegisterTable[insn.Rsx[1].R]);
            break;

        case XOps.Rem:
            p_RegisterTable[insn.Rd.R] = (ulong)((long)p_RegisterTable[insn.Rsx[0].R] % (long)p_RegisterTable[insn.Rsx[1].R]);
            break;

        }
    }

    public override unsafe XCallObject<CallFrame>? ProcessInsn(XMethod func, ref XTargetState<CallFrame> state, ref int ip, ulong last_return_value)
    {
        var insn = func.GetInsn(ip);
        var frame = state.Frame;
        var insn_type = insn.Type;

        XCallObject<CallFrame>? branched_function_target = null;

        // little side gig here: set return value from previous instruction
        if (frame.ReturnRegister != -1)
        {
            frame.RegisterTable[frame.ReturnRegister] = state.ReturnValue;
            frame.ReturnRegister = -1;
        }

        fixed (ulong* p_RegisterTable = frame.RegisterTable)
        {
            Span<ulong> Register_Restore = stackalloc ulong[frame.RegisterTable.Length];
            Register_Restore.Fill(0);

            if (BitConverter.IsLittleEndian)
            {
                if (insn_type.Size == 1)
                {
                    for (var i = 0; i < frame.RegisterTable.Length; i++)
                    {
                        Register_Restore[i] = p_RegisterTable[i] & 0xFFFF_FFFF_FFFF_FF00;
                        p_RegisterTable[i] &= 0xFF;
                    }
                }
                else if (insn_type.Size == 2)
                {
                    for (var i = 0; i < frame.RegisterTable.Length; i++)
                    {
                        Register_Restore[i] = p_RegisterTable[i] & 0xFFFF_FFFF_FFFF_0000;
                        p_RegisterTable[i] &= 0xFFFF;
                    }
                }
                else if (insn_type.Size == 4)
                {
                    for (var i = 0; i < frame.RegisterTable.Length; i++)
                    {
                        Register_Restore[i] = p_RegisterTable[i] & 0xFFFF_FFFF_0000_0000;
                        p_RegisterTable[i] &= 0xFFFF_FFFFF;
                    }
                }
            }
            else
            {
                if (insn_type.Size == 1)
                {
                    for (var i = 0; i < frame.RegisterTable.Length; i++)
                    {
                        Register_Restore[i] = p_RegisterTable[i] & 0x00FF_FFFF_FFFF_FFFF;
                        p_RegisterTable[i] &= 0xFF00_0000_0000_0000;
                    }
                }
                else if (insn_type.Size == 2)
                {
                    for (var i = 0; i < frame.RegisterTable.Length; i++)
                    {
                        Register_Restore[i] = p_RegisterTable[i] & 0x0000_FFFF_FFFF_FFFF;
                        p_RegisterTable[i] &= 0xFFFF_0000_0000_0000;
                    }
                }
                else if (insn_type.Size == 4)
                {
                    for (var i = 0; i < frame.RegisterTable.Length; i++)
                    {
                        Register_Restore[i] = p_RegisterTable[i] & 0x0000_0000_FFFF_FFFF;
                        p_RegisterTable[i] &= 0xFFFF_FFFF_0000_0000;
                    }
                }
            }

            var covered = true;
            switch (insn.Op)
            {

            // SPECIAL

            case XOps.Nop:
                break;

            case XOps.Break:
                state.Sig_Abort = true;
                break;

            case XOps.Asm: /* replaced as a no-op */
                Diagnostic.Throw(null, 0, DiagnosticSeverity.Info, "xrun-asm-block", "encountered assembly block in xrun. aborted.");
                state.Sig_Abort = true;
                break;

            // REGISTER CONTROL OPERATIONS

            case XOps.ConstSet:
                p_RegisterTable[insn.Rd.R] = (ulong)insn.Kx[0];
                break;

            case XOps.Set:
                p_RegisterTable[insn.Rd.R] = p_RegisterTable[insn.Rsx[0].R];
                break;

            // MEMORY OPERATIONS

            case XOps.Read:
                p_RegisterTable[insn.Rd.R] = *(ulong*)p_RegisterTable[insn.Rsx[0].R];
                break;

            case XOps.Write:
                *(ulong*)p_RegisterTable[insn.Rd.R] = p_RegisterTable[insn.Rsx[0].R];
                break;

            case XOps.Copy:
                *(ulong*)p_RegisterTable[insn.Rd.R] = *(ulong*)p_RegisterTable[insn.Rsx[0].R];
                break;

            case XOps.AddressOf:
                p_RegisterTable[insn.Rd.R] = (ulong)p_RegisterTable + (uint)insn.Rsx[0].R * sizeof(ulong);
                break;

            // BLOCK OPERATIONS

            case XOps.Blkzero:
                new Span<byte>((void*)p_RegisterTable[insn.Rd.R], (int)insn.Kx[0]).Fill(0);
                break;

            case XOps.Blkcopy:
                Buffer.MemoryCopy((void*)p_RegisterTable[insn.Rsx[0].R], (void*)p_RegisterTable[insn.Rd.R], (long)insn.Kx[0], (long)insn.Kx[0]);
                break;

            case XOps.Blkmove:
                Buffer.MemoryCopy((void*)p_RegisterTable[insn.Rsx[0].R], (void*)p_RegisterTable[insn.Rd.R], (long)insn.Kx[0], (long)insn.Kx[0]);
                break;

            case XOps.Blkaddr:
                p_RegisterTable[insn.Rd.R] = (ulong)frame.Blocks[(int)insn.Kx[0]];
                break;

            // CONTROL OPERATIONS

            case XOps.Jmp:
                ip = (int)insn.Kx[0];
                break;

            case XOps.IJmp:
            case XOps.PiCall:
            case XOps.FiCall:
                Diagnostic.Throw(null, 0, DiagnosticSeverity.Info, "xrun-indirect-branch", "indirect branches are not supported by xrun. aborted.");
                state.Sig_Abort = true;
                break;

            case XOps.If:
                if (p_RegisterTable[insn.Rd.R] != 0)
                    ip = (int)insn.Kx[0];
                break;

            case XOps.Ifz:
                if (p_RegisterTable[insn.Rd.R] == 0)
                    ip = (int)insn.Kx[0];
                break;

            case XOps.Leave:
                state.Sig_Return = true;
                break;

            // TODO: most likely slow as dogwater
            case XOps.Ret:
                ip = func.GetLabel("$leave");
                break;

            // TODO: most likely slow as dogwater
            case XOps.Retv:
                state.ReturnValue = p_RegisterTable[insn.Rd.R];
                ip = func.GetLabel($"leave");
                break;

            case XOps.FCall:
            {
                var target = func.Module.GetMethod(insn.Fn ?? "");
                Debug.Assert(target is not null);

                var param_list = new ulong[insn.Rsx.Length];
                for (var i = 0; i < insn.Rsx.Length; i++)
                    param_list[i] = p_RegisterTable[insn.Rsx[i].R];

                frame.ReturnRegister = insn.Rd.R;
                branched_function_target = new(target, 0, new() { Params = param_list });

                // all should be in order here
                break;
            }

            case XOps.PCall:
            {
                var target = func.Module.GetMethod(insn.Fn ?? "");
                Debug.Assert(target is not null);

                var param_list = new ulong[insn.Rsx.Length];
                for (var i = 0; i < insn.Rsx.Length; i++)
                    param_list[i] = p_RegisterTable[insn.Rsx[i].R];
                
                // VVV --- what makes PCall different --- VVVV
                //frame.ReturnRegister = insn.Rd.R; <-- this is off
                //////////////////////////////////////////////
                branched_function_target = new(target, 0, new() { Params = param_list });

                // all should be in order here
                break;
            }

            // BITWISE OPERATIONS

            case XOps.BNot:
                p_RegisterTable[insn.Rd.R] = ~p_RegisterTable[insn.Rsx[0].R];
                break;

            case XOps.BAnd:
                p_RegisterTable[insn.Rd.R] = p_RegisterTable[insn.Rsx[0].R] & p_RegisterTable[insn.Rsx[1].R];
                break;

            case XOps.BOr:
                p_RegisterTable[insn.Rd.R] = p_RegisterTable[insn.Rsx[0].R] | p_RegisterTable[insn.Rsx[1].R];
                break;

            case XOps.BXor:
                p_RegisterTable[insn.Rd.R] = p_RegisterTable[insn.Rsx[0].R] ^ p_RegisterTable[insn.Rsx[1].R];
                break;

            case XOps.LNot:
                p_RegisterTable[insn.Rd.R] = p_RegisterTable[insn.Rsx[0].R] == 0 ? 1UL : 0UL;
                break;

            case XOps.ALsh:
                p_RegisterTable[insn.Rd.R] = (ulong)((long)p_RegisterTable[insn.Rsx[0].R] << (byte)p_RegisterTable[insn.Rsx[1].R]);
                break;

            case XOps.ARsh:
                p_RegisterTable[insn.Rd.R] = (ulong)((long)p_RegisterTable[insn.Rsx[0].R] >> (byte)p_RegisterTable[insn.Rsx[1].R]);
                break;

            case XOps.BLsh:
                p_RegisterTable[insn.Rd.R] = p_RegisterTable[insn.Rsx[0].R] << (byte)p_RegisterTable[insn.Rsx[1].R];
                break;

            case XOps.BRsh:
                p_RegisterTable[insn.Rd.R] = p_RegisterTable[insn.Rsx[0].R] >> (byte)p_RegisterTable[insn.Rsx[1].R];
                break;

                // TODO: MEMORY BLOCKS

            /*case XOps.Icast:
            {
                var src_type = insn.Tsx[0];
                var rsx = insn.Rsx[0].R;
                var result = p_RegisterTable[rsx] | Register_Restore[rsx];
                var sign = false;

                switch (src_type.Size)
                {

                case 2:
                    if (BitConverter.IsLittleEndian)
                    {
                        sign = (result & 0x8000) == 1;
                        result &= 0x0000_0000_0000_FFFF;
                    }
                    else
                    {
                        result = (result & 0xFFFF_0000_0000_0000) >> 48;
                        sign = (result & 0x8000_0000_0000_0000) == 1;
                    }

                    result = insn_type.Size switch
                    {
                        1 =>
                            insn_type.Kind == XTypeKind.S ? (byte)(sbyte)((short)result * (sign ? -1 : 1))
                                                          : (byte)(ushort)((short)result * (sign ? -1 : 1)),
                        2 =>
                            insn_type.Kind == XTypeKind.S ? result : (ushort)((short)result * (sign ? -1 : 1)),
                    };
                    break;

                case 4:
                    if (BitConverter.IsLittleEndian)
                    {
                        result &= 0x0000_0000_FFFF_FFFF;
                        sign = (result & 0x8000_0000) == 1;
                    }
                    else
                    {
                        result = (result & 0xFFFF_FFFF_0000_0000) >> 32;
                        sign = (result & 0x8000_0000_0000_0000) == 1;
                    }

                    // don't do anything besides that as far as casting is concerned
                    if (insn_type.Kind == XTypeKind.U)
                        break;

                    result = (uint)((int)result * (sign ? -1 : 1));
                    break;
                }

                p_RegisterTable[insn.Rd.R] = result;
            }*/

                // TODO: int->float, float->int, float_resize, icast

            default:
                covered = false;
                break;
            }

            if (covered)
                goto LSkipArithmOperators;

            switch (insn_type.Kind)
            {

            case XTypeKind.Vs:
            case XTypeKind.Vd:
                Diagnostic.Throw(null, 0, DiagnosticSeverity.Info, "xrun-vector", "vector types are not supported. aborted.");
                state.Sig_Abort = true;
                break;

            case XTypeKind.Fp:
                ProcessFloatInsn(func, ref state, ref ip, p_RegisterTable);
                break;

            case XTypeKind.U:
                ProcessUnsignedInsn(func, ref state, ref ip, p_RegisterTable);
                break;

            case XTypeKind.S:
                ProcessSignedInsn(func, ref state, ref ip, p_RegisterTable);
                break;

            default:
                // something is wrong if we end up here
                throw new NotImplementedException();

            }

LSkipArithmOperators:
            // restore the registers
            for (var i = 0; i < frame.RegisterTable.Length; i++)
                p_RegisterTable[i] |= Register_Restore[i];
        }

        return branched_function_target;
    }
}
