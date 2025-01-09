namespace ck.Lang.Type;

/// <summary>
/// Stores the type subjugate's signature.
/// </summary>
public interface ITypeSubjugateSignature
{
    /// <summary>
    /// Gets a list of subjugate types represented by the
    /// signature.
    /// </summary>
    public IEnumerable<FType> GetSubjugateTypes();

    /// <summary>
    /// Result of lengthof().
    /// </summary>
    public nint LengthOf();

    /// <summary>
    /// Compares two <see cref="ITypeSubjugateSignature"/>. This is not 100% accurate before
    /// all user types are resolved. Food assumes they are then not equal.
    /// </summary>
    /// <param name="other">It is safe to cast this to the type of the implementing class,
    /// because a type check is already befored on both <see cref="ITypeSubjugateSignature"/>s.</param>
    public bool EqualTo(ITypeSubjugateSignature other);
}
