module(gen_common)
  function(main)
    blocks {
    }
    code {
      0  copy::U64 Rd=3, Rsx=[2]
      1  increment::U64 Rd=3, Kx=[0]
      2  constset::Fp32 Rd=4, Kx=[4607182418800017408]
      3  write::U0 Rd=3, Rsx=[4]
      4  copy::U64 Rd=5, Rsx=[2]
      5  increment::U64 Rd=5, Kx=[4]
      6  read::Fp32 Rd=5, Rsx=[5]
      7  constset::S32 Rd=6, Kx=[0]
      8  notequal::S8 Rd=5, Rsx=[5, 6]
      9  if::U0 Rd=5, Kx=[11]
      10  jmp::U0 Rd=-1, Kx=[13]
      11  constset::S32 Rd=7, Kx=[2]
      12  retv::S32 Rd=7
      13  leave::U0 Rd=-1
    }
  end function
  function(__ck_impl_strlen)
    blocks {
    }
    code {
      0  addressof::U64 Rd=2, Rsx=[1]
      1  constset::U64 Rd=3, Kx=[0]
      2  write::U0 Rd=2, Rsx=[3]
      3  addressof::U64 Rd=6, Rsx=[0]
      4  read::U64 Rd=7, Rsx=[6]
      5  copy::U64 Rd=5, Rsx=[7]
      6  increment::U64 Rd=7, Kx=[1]
      7  write::U64 Rd=6, Rsx=[7]
      8  read::U8 Rd=4, Rsx=[5]
      9  if::U0 Rd=4, Kx=[11]
      10  jmp::U0 Rd=-1, Kx=[17]
      11  addressof::U64 Rd=9, Rsx=[1]
      12  read::U64 Rd=10, Rsx=[9]
      13  increment::U64 Rd=10, Kx=[1]
      14  copy::U64 Rd=8, Rsx=[10]
      15  write::U64 Rd=9, Rsx=[10]
      16  jmp::U0 Rd=-1, Kx=[3]
      17  copy::U64 Rd=11, Rsx=[1]
      18  retv::U64 Rd=11
      19  leave::U0 Rd=-1
    }
  end function
  function(__ck_impl_assert_abort)
    blocks {
    }
    code {
      0  asm::U0 Rd=-1, Kx=[4170793894]
      1  leave::U0 Rd=-1
    }
  end function
  function(__ck_impl_memcmp)
    blocks {
    }
    code {
      0  copy::U64 Rd=6, Rsx=[3]
      1  copy::U64 Rd=8, Rsx=[0]
      2  icast::U64 Rd=7, Rsx=[8]
      3  write::U0 Rd=6, Rsx=[7]
      4  copy::U64 Rd=9, Rsx=[4]
      5  copy::U64 Rd=11, Rsx=[1]
      6  icast::U64 Rd=10, Rsx=[11]
      7  write::U0 Rd=9, Rsx=[10]
      8  addressof::U64 Rd=12, Rsx=[5]
      9  constset::S32 Rd=13, Kx=[0]
      10  write::U0 Rd=12, Rsx=[13]
      11  constset::S8 Rd=14, Kx=[1]
      12  retv::S8 Rd=14
      13  leave::U0 Rd=-1
    }
  end function
  function(__ck_impl_member_of)
    blocks {
    }
    code {
      0  addressof::U64 Rd=5, Rsx=[4]
      1  constset::S32 Rd=6, Kx=[0]
      2  write::U0 Rd=5, Rsx=[6]
      3  constset::S8 Rd=7, Kx=[0]
      4  retv::S8 Rd=7
      5  leave::U0 Rd=-1
    }
  end function
end module
