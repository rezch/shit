/**
 * @file shit tree sitter parser
 * @author halo
 * @license MIT
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

module.exports = grammar({
  name: "shit",

  rules: {
    source_file: $ => repeat($._definition),

    _definition: $ => choice(
      $.comment,
      $.extern_definition,
      $.function_definition,
      $.call,
      $.return_statement,
      $._expression,
      $.ifelse,
      $.for_loop,
      $.eol,
    ),

    ifelse: $ => seq(
      'if',
      $._expression,
      ':',
      $._expression,
      optional(
        seq(
          'else',
          $._expression
        )
      )
    ),

    for_loop: $ => seq(
      'for',
      '(',
      $.identifier, '=', $._expression, ';',
      $._expression,
      optional(
        seq(
          ';',
          $._expression
        ),
      ),
      ')'
    ),

    comment: $ => /#.*/,

    extern_definition: $ => seq(
      'extern',
      field('module', $.call),
    ),

    function_definition: $ => seq(
      'fun',
      field('name', $.identifier),
      field('parameters', $.parameter_list),
    ),

    call: $ => seq(
      field('name', $.identifier),
      field('args', $.args_list),
    ),

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

    return_statement: $ => seq(
      'ret',
      $._expression,
    ),

    _expression: $ => choice(
      $.identifier,
      $.number,
      prec(2, $.call),
      $.binary_expression,
    ),

    binary_expression: $ => choice(
      prec.left(2, seq($._expression, '+', $._expression)),
      prec.left(2, seq($._expression, '-', $._expression)),
      prec.left(1, seq($._expression, '*', $._expression)),
      prec.left(1, seq($._expression, '/', $._expression)),

      prec.left(0,
        seq(
          choice(
            $.identifier,
          ),
          '=',
          $._expression
        )
      ),

      prec.left(1,
        seq(
          choice(
            $.identifier,
          ),
          choice('>', '<'),
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

    number: $ => /\d+|-\d+/,

    eol: $ => ';',
  }
});
