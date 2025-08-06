/**
 * @file shit tree sitter parser
 * @author halo
 * @license MIT
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

module.exports = grammar({
  name: "shit",

  // word: $ => $.identifier,

  rules: {
    source_file: $ => repeat($._definition),

    _definition: $ => choice(
      $.comment,
      $.extern_definition,
      $.function_definition,
      $.call,
      // $.var_definition,
      $.return_statement,
      $._expression,
    ),

    comment: $ => /#.*/,

    extern_definition: $ => seq(
      'extern',
      field('module', $.identifier),
    ),

    function_definition: $ => seq(
      'fun',
      field('name', $.identifier),
      field('parameters', $.parameter_list),
      field('body', $.block),
    ),

    block: $ => seq(
      '{',
      repeat($._definition),
      '}'
    ),

    call: $ => seq(
      field('name', $.identifier),
      field('args', $.args_list),
    ),

    // var_definition: $ => seq(
    //   $._type,
    //   $.identifier,
    // ),

    parameter_list: $ => seq(
      '(',
      field('arg',
        repeat(
          seq(
            $.identifier,
            optional(','),
          ),
        ),
      ),
      ')'
    ),

    args_list: $ => seq(
      '(',
      field('arg_value',
        repeat(
          seq(
            $._expression,
            optional(',')
          ),
        ),
      ),
      ')'
    ),

    // _type: $ => choice(
    //   $.primitive_type,
    // ),

    // primitive_type: $ => choice(
    //   'bool',
    //   'int',
    //   'float',
    //   'str',
    // ),

    // _statement: $ => choice(
    //   $.return_statement
    // ),

    return_statement: $ => seq(
      'ret',
      $._expression,
    ),

    _expression: $ => choice(
      // $.bool,
      $.identifier,
      $.number,
      prec(2, $.call),
      // $.float_number,
      // $.string,
      $.binary_expression,
    ),

    binary_expression: $ => choice(
      prec.left(1, seq($._expression, '+', $._expression)),
      prec.left(1, seq($._expression, '-', $._expression)),
      prec.left(2, seq($._expression, '*', $._expression)),
      prec.left(2, seq($._expression, '/', $._expression)),

      prec.left(0,
        seq(
          choice(
            // $.var_definition,
            $.identifier,
          ),
          '=',
          $._expression
        )
      ),
    ),

    identifier: $ => prec(1, /[a-zA-Z][a-zA-Z0-9]*/),

    // bool: $ => prec(
    //   2,
    //   choice(
    //     'true',
    //     'false'
    //   ),
    // ),

    number: $ => /\d+/,

    // float_number: $ => /[+-]?((\d+\.?\d*)|(\.\d+))/,

    // string: $ => /".*"/
  }
});
