module(gen_common)
  function(main)
    blocks {
      [0]=(at=+0x0, sz=16)
    }
    code {
      0  copy::U64 Rd=3, Rsx=[2]
      1  blkaddr::U64 Rd=4, Kx=[0]
      2  copy::U64 Rd=5, Rsx=[4]
      3  copy::S32 Rd=6, Rsx=[0]
      4  write::S32 Rd=5, Rsx=[6]
      5  increment::U64 Rd=5, Kx=[4]
      6  copy::S32 Rd=7, Rsx=[0]
      7  constset::S32 Rd=8, Kx=[1]
      8  add::S32 Rd=7, Rsx=[7, 8]
      9  write::S32 Rd=5, Rsx=[7]
      10  increment::U64 Rd=5, Kx=[4]
      11  copy::S32 Rd=9, Rsx=[0]
      12  constset::S32 Rd=10, Kx=[2]
      13  add::S32 Rd=9, Rsx=[9, 10]
      14  write::S32 Rd=5, Rsx=[9]
      15  increment::U64 Rd=5, Kx=[4]
      16  copy::S32 Rd=11, Rsx=[0]
      17  constset::S32 Rd=12, Kx=[3]
      18  add::S32 Rd=11, Rsx=[11, 12]
      19  write::S32 Rd=5, Rsx=[11]
      20  increment::U64 Rd=5, Kx=[4]
      21  write::U0 Rd=3, Rsx=[4]
      22  leave::U0 Rd=-1
    }
  end function
  function(__ck_impl_strlen)
    blocks {
    }
    code {
      0  addressof::U64 Rd=2, Rsx=[1]
      1  constset::S32 Rd=4, Kx=[0]
      2  icast::U64 Rd=3, Rsx=[4]
      3  write::U0 Rd=2, Rsx=[3]
      4  addressof::U64 Rd=7, Rsx=[0]
      5  read::U64 Rd=8, Rsx=[7]
      6  copy::U64 Rd=6, Rsx=[8]
      7  increment::U64 Rd=8, Kx=[1]
      8  write::U64 Rd=7, Rsx=[8]
      9  read::U8 Rd=5, Rsx=[6]
      10  if::U0 Rd=5, Kx=[12]
      11  jmp::U0 Rd=-1, Kx=[18]
      12  addressof::U64 Rd=10, Rsx=[1]
      13  read::U64 Rd=11, Rsx=[10]
      14  increment::U64 Rd=11, Kx=[1]
      15  copy::U64 Rd=9, Rsx=[11]
      16  write::U64 Rd=10, Rsx=[11]
      17  jmp::U0 Rd=-1, Kx=[4]
      18  copy::U64 Rd=12, Rsx=[1]
      19  retv::U64 Rd=12
      20  leave::U0 Rd=-1
    }
  end function
  function(__ck_impl_assert_abort)
    blocks {
    }
    code {
      0  asm::U0 Rd=-1, Kx=[1272080378]
      1  leave::U0 Rd=-1
    }
  end function
end module
