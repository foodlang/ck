namespace ck.Lang.Type;

/// <summary>
/// The subjugate signature of an array.
/// </summary>
public sealed class ArraySubjugateSignature : ITypeSubjugateSignature
{
    /// <summary>
    /// The type of the pointed object.
    /// </summary>
    public FType ObjectType;

    /// <summary>
    /// The length of the array. If set to 0, means unknown length.
    /// </summary>
    public readonly nint Length;

    public IEnumerable<FType> GetSubjugateTypes() => [ObjectType];

    public nint LengthOf() => Length;

    public bool EqualTo(ITypeSubjugateSignature other)
    {
        var other_as_array = (ArraySubjugateSignature)other;
        return ObjectType != other_as_array.ObjectType
            ? false
            : Length == other_as_array.Length;
    }

    public void WriteSubjugateTypes(IEnumerable<FType> subjugate_types)
    {
        ObjectType = subjugate_types.ElementAt(0);
    }

    public ArraySubjugateSignature(FType object_type, nint length)
    {
        ObjectType = object_type;
        Length = length;
    }
}
