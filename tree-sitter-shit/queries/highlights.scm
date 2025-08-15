; Keywords

[
 "extern"
 "fun"
 "ret"
 "if"
 "else"
 "for"
] @keyword

; Identifiers
(identifier) @variable
(number) @number
(extern_definition module: (call) @module)

; Comments
(comment) @comment

; Functions
(function_definition
  name: (identifier) @function
  parameters: (parameter_list) @punctuation.bracket)

arg: (identifier) @variable.parameter
arg_value: (identifier) @variable

(call
  name: (identifier) @function.call
  args: (args_list) @punctuation.bracket)
