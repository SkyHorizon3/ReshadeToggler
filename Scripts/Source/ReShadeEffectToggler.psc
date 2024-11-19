Scriptname ReShadeEffectToggler Hidden

; returns whether ReShade with full add-on support is found
Bool Function IsReShadeInstalled() global native

; Toggle Effect on/off
; Example: ReShadeEffectToggler.ToggleEffect("Colourfulness.fx", true)
; Checks whether ReShade with full add-on support and the effect are available
Function ToggleEffect(String effectName, bool toggleState) global native

; Toggle ReShade on/off
; Checks whether ReShade with full add-on support is installed
Function ToggleReShade(bool toggleState) global native