using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ck.XIL;

/// <summary>
/// Universal Food IL (UFIL) opcodes
/// </summary>
public enum XOps : byte
{
    /*
     *
     * nop            ; no-op
     * break          ; breakpoint or other weird operation
     *
     * Rd  = K0       ; constant set
     * Rd  = Rs0      ; set
     * Rd z= Rs0      ; zero-extend Rs0 into Rd
     * Rd s= Rs0      ; sign-extend Rs0 into Rd
     * Rd  = *Rs0     ; read
     * *Rd = Rs0      ; write
     * *Rd = *Rs0     ; copy
     * Rd  = &Rs0     ; address of (requires variable or block)
     * 
     * Rd = Rs0 + K0  ; increment
     * Rd = Rs0 - K0  ; decrement
     * Rd = Rs0 * K0  ; multiply const
     * Rd = Rs0 / K0  ; divide const
     * 
     * Rd = -Rs0      ; unary minus
     * Rd = Rs0 + Rs1 ; addition
     * Rd = Rs0 - Rs1 ; subtraction
     * Rd = Rs0 * Rs1 ; multiplication
     * Rd = Rs0 / Rs1 ; division
     * Rd = Rs0 % Rs1 ; remainder
     * 
     * Rd = ~Rs0       ; bitwise NOT
     * Rd = Rs0 & Rs1  ; bitwise AND
     * Rd = Rs0 ^ Rs1  ; bitwise XOR
     * Rd = Rs0 | Rs1  ; bitwise OR
     * 
     * Rd = Rs0 << Rs1  ; bitwise left shift
     * Rd = Rs0 >> Rs1  ; bitwise right shift
     * Rd = Rs0 <<a Rs1 ; arithmetic left shift
     * Rd = Rs0 >>a Rs1 ; arithmetic right shift
     * 
     * Rd = Rs0 < Rs1  ; lower
     * Rd = Rs0 <= Rs1 ; lower equal
     * Rd = Rs0 > Rs1  ; greater
     * Rd = Rs0 >= Rs1 ; greater equal
     * Rd = Rs0 == Rs1 ; equal
     * Rd = Rs0 != Rs1 ; not equal
     * Rd = !Rs0       ; logical not
     * 
     * jmp InsnOffset       ; jumps to an offset
     * ijmp Rd              ; indirect jump
     * if Rd -> InsnOffset  ; if Rd != 0, jump to InsnOffset
     * ifz Rd -> InsnOffset ; if Rd == 0, jump to InsnOffset
     * 
     * Rd = fcall F [Rs...]     ; calls a function
     * pcall F [Rs...]          ; calls a procedure
     * Rd = ficall Rs0 [Rs1...] ; calls indirectly a function
     * picall Rs0 [Rs1...]      ; calls indirectly a procedure
     * retv Rd                  ; returns a value (soft return)
     * ret                      ; returns (with no value) (soft return)
     * leave                    ; leaves the function [true return]. value is preserved from last retv
     * 
     * blkzero(K0) Rd      ; initializes the block to zero at Rd (K0 bytes)
     * blkcopy(K0) Rd, Rs0 ; copies block from Rs0 to Rd (K0 bytes), overlap not permitted. analoguous to memcpy()
     * blkmove(K0) Rd, Rs0 ; copies block from Rs0 to Rd (K0 bytes), allows overlap. analoguous to memmove()
     * blkaddr(K0) Rd      ; makes a local block address
     * 
     * Rd = float(Rs0) ; int->float convert (4->4, 8->8)
     * Rd = int(Rs0)   ; float->int convert (4->4, 8->8)
     * Rd = float_resize Rs0 ; resizes a float
     * Rd = icast Rs0  ; convert integer types
     *
    **/

    Nop,
    Break,

    ConstSet,
    Set,
    ZExt,
    SExt,
    Read,
    Write,
    Copy,
    AddressOf,

    Increment,
    Decrement,
    MulConst,
    DivConst,

    UnaryMinus,
    Add,
    Sub,
    Mul,
    Div,
    Rem,

    BNot,
    BAnd,
    BXor,
    BOr,

    BLsh,
    BRsh,
    ALsh,
    ARsh,

    Lower,
    LowerEqual,
    Greater,
    GreaterEqual,
    Equal,
    NotEqual,
    LNot,

    Jmp,
    IJmp,
    If,
    Ifz,

    FCall,
    PCall,
    FiCall,
    PiCall,
    Retv,
    Ret,
    Leave,

    Blkzero,
    Blkcopy,
    Blkmove,
    Blkaddr,

    IntToFloat,
    FloatToInt,
    FloatResize,
    Icast,
}
