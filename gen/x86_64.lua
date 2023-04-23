-- ****** Register allocation ******

local registers = {
	{ used = false, b = "bl", w = "bx", dw = "ebx", qw = "rbx" },
	{ used = false, b = "cl", w = "cx", dw = "ecx", qw = "rcx" },
	{ used = false, b = "dl", w = "dx", dw = "edx", qw = "rdx" },
	{ used = false, b = "r8b", w = "r8w", dw = "r8d", qw = "r8" },
	{ used = false, b = "r9b", w = "r9w", dw = "r9d", qw = "r9" },
	{ used = false, b = "r10b", w = "r10w", dw = "r10d", qw = "r10" },
	{ used = false, b = "r11b", w = "r11w", dw = "r11d", qw = "r11" },
	{ used = false, b = "r12b", w = "r12w", dw = "r12d", qw = "r12" },
	{ used = false, b = "r13b", w = "r13w", dw = "r13d", qw = "r13" },
	{ used = false, b = "r14b", w = "r14w", dw = "r14d", qw = "r14" },
	{ used = false, b = "r15b", w = "r15w", dw = "r15d", qw = "r15" },
}

function regalloc()
	for i, k in ipairs(registers) do
		if not k.used then
			k.used = true
			return i
		end
	end
end

function regfree(i)
	if i != 255 then
		registers[i].used = false
	end
end

-- ******************************

local expracc = ""

function genexpr(expression)
	left = expression.left != nil and genexpr(expression.left) or 255
	right = expression.right != nil and genexpr(expression.right) or 255
end

function ckgen_expression(expression)
	expracc = ""
	regfree(genexpr)
	return expracc
end