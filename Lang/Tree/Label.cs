using ck.Lang.Tree.Statements;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.Lang.Tree;

/// <summary>
/// A label in a statement.
/// </summary>
public sealed record Label(string Name, int Index);
