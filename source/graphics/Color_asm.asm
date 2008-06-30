%include "../lib/sysdep/arch/ia32/ia32.inc"

;-------------------------------------------------------------------------------
; Color conversion (SSE)
;-------------------------------------------------------------------------------

; extern "C" u32 ConvertRGBColorTo4ub(const RGBColor& color)
[section .data]
	align	16
zero:
	dd	0.0
twofivefive:
	dd	255.0


__SECT__
	align	16
global sym(sse_ConvertRGBColorTo4ub)
sym(sse_ConvertRGBColorTo4ub):
	mov	eax, [esp+4]

	; xmm0, 1, 2 = R, G, B
	movss	xmm4, [zero]
	movss	xmm0, [eax+8]
	movss	xmm1, [eax+4]
	movss	xmm2, [eax]
	movss	xmm5, [twofivefive]

	; C = min(255, 255*max(C, 0)) ( == clamp(255*C, 0, 255) )
	maxss	xmm0, xmm4
	maxss	xmm1, xmm4
	maxss	xmm2, xmm4
	mulss	xmm0, xmm5
	mulss	xmm1, xmm5
	mulss	xmm2, xmm5
	minss	xmm0, xmm5
	minss	xmm1, xmm5
	minss	xmm2, xmm5

	; convert to integer and combine channels using bit logic
	cvtss2si eax, xmm0
	cvtss2si ecx, xmm1
	cvtss2si edx, xmm2
	shl	eax, 16
	shl	ecx, 8
	or	eax, 0xff000000
	or	edx, ecx
	or	eax, edx

	ret
