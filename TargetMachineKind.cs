namespace ck;

public static class TargetMachine
{
    public enum CallConv
    {
        intrin,
        cmptime,

        cdecl,
        sysv64,
        win64,
        aarch64,
    }

    public enum Architecture
    {
        cmptime,
        i386,
        amd64,
        aarch64,
    }

    public enum ArchitectureExt
    {
        sse,
        sse2,
        avx,
        avx512,
    }

    public enum BinaryType
    {
        cmptime,
        elf,
        win_pe,
        bin,

        shared_elf,
        shared_pe,

        static_elf,
        static_pe,
    }

    public enum System
    {
        windows,
        darwin,
        x,
    }

    public static CallConv CallingConvention = CallConv.sysv64;
    public static Architecture Arch = Architecture.amd64;
    public static ArchitectureExt ArchExts = ArchitectureExt.sse2;
    public static BinaryType Bin = BinaryType.elf;
    public static System Sys = System.x;

    public static short PointerSize()
        => Arch switch
        {
            Architecture.i386 => 4,
            Architecture.aarch64 => 8,
            Architecture.amd64 => 8,
            Architecture.cmptime => 8,
            _ => throw new NotImplementedException()
        };

    public static bool InlineAssemblyAllowed() => Arch != Architecture.cmptime;
}
