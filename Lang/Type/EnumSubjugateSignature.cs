namespace ck.Lang.Type;

public sealed class EnumSubjugateSignature : ITypeSubjugateSignature
{
    public readonly FType Underlying;

    public bool EqualTo(ITypeSubjugateSignature other)
    {
        if (other is EnumSubjugateSignature @enum)
            return Underlying == @enum.Underlying;
        return false;
    }

    public IEnumerable<FType> GetSubjugateTypes() => [Underlying];
    public nint LengthOf() => 0;

    public EnumSubjugateSignature(FType underlying)
        => Underlying = underlying;
}
