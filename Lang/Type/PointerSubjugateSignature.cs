namespace ck.Lang.Type;

/// <summary>
/// The subjugate signature of a pointer.
/// </summary>
public sealed class PointerSubjugateSignature : ITypeSubjugateSignature
{
    /// <summary>
    /// The type of the pointed object.
    /// </summary>
    public FType ObjectType;

    public IEnumerable<FType> GetSubjugateTypes() => new[] { ObjectType };

    // no length, can be used as a buffer or as simply a pointer to a single object.
    // we don't what kind of hacks... or voodoo the user aims to do, but we're rocking
    // with it!!!
    public nint LengthOf() => 0;

    public bool EqualTo(ITypeSubjugateSignature other) =>
        ObjectType == ((PointerSubjugateSignature)other).ObjectType;
    public void WriteSubjugateTypes(IEnumerable<FType> subjugate_types) =>
        ObjectType = subjugate_types.ElementAt(0);

    public PointerSubjugateSignature(FType object_type)
    {
        ObjectType = object_type;
    }
}
