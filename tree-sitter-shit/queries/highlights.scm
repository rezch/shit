; Keywords

[
 "extern"
 "fun"
 "ret"
] @keyword

; Types
(primitive_type) @type

; Identifiers
(identifier) @variable
(bool) @boolean
(number) @number
(float_number) @number.float
(string) @string

; Comments
(comment) @comment

; Functions
(function_definition name: (identifier) @function)
(call name: (identifier) @function.call)
