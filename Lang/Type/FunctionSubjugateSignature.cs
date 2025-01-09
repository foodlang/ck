using ck.Lang.Tree.Scopes;
using ck.Util;

namespace ck.Lang.Type;

/// <summary>
/// The subjugate signature of a function or function callback.
/// </summary>
public sealed class FunctionSubjugateSignature : ITypeSubjugateSignature
{
    /// <summary>
    /// The return type of the function.
    /// </summary>
    public FType ReturnType;

    /// <summary>
    /// The arguments of the function
    /// </summary>
    public FunctionArgumentSignature Params;

    public IEnumerable<FType> GetSubjugateTypes() => new[] { ReturnType }.Concat(Params.Arguments.Select(a => a.Type));

    public nint LengthOf() => 1;
    public bool EqualTo(ITypeSubjugateSignature other)
    {
        var other_function = (FunctionSubjugateSignature)other;

        return ReturnType != other_function.ReturnType
            ? false
            : Params.VariadicType != other_function.Params.VariadicType
            ? false
            : Params.Arguments.Count() != other_function.Params.Arguments.Count()
                ? false
                : Params.Arguments.FullMatch(other_function.Params.Arguments);
    }

    public FunctionSubjugateSignature(FType returnType, FunctionArgumentSignature arguments)
    {
        ReturnType = returnType;
        Params = arguments;
    }
}
