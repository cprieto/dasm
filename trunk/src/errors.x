/*
    $Id$

    the DASM macro assembler (aka small systems cross assembler)

    Copyright (c) 1988-2002 by Matthew Dillon.
    Copyright (c) 1995 by Olaf "Rhialto" Seibert.
    Copyright (c) 2003-2008 by Andrew Davie.
    Copyright (c) 2008 by Peter H. Froehlich.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/**
 * @brief Use the X-macro trick to define error codes and tables.
 *
 * @see http://www.ddj.com/cpp/184401387
 *
 * @warning Format is as follows: X(code, fatal, message) for now.
 *
 * @todo Distinguish error levels, introduce generic message on each
 * level, refactor, etc.
 */

/* core DASM errors */
X(ERROR_NONE, true, "OK")
X(ERROR_COMMAND_LINE, true, "Check command-line format.")
X(ERROR_FILE_ERROR, true, "Unable to open file.")
X(ERROR_NOT_RESOLVABLE, true, "Source is not resolvable.") /* ??? [phf] */
X(ERROR_TOO_MANY_PASSES, true, "Too many passes (%s).") /* ??? [phf] */
X(ERROR_SYNTAX_ERROR, true, "Syntax error '%s'.")
X(ERROR_EXPRESSION_TABLE_OVERFLOW, true, "Expression table overflow.")
X(ERROR_UNBALANCED_BRACES, true, "Unbalanced braces [].") /* ??? [phf] */
X(ERROR_DIVISION_BY_0, true, "Division by zero.")
X(ERROR_UNKNOWN_MNEMONIC, true, "Unknown mnemonic '%s'.")
X(ERROR_ILLEGAL_ADDRESSING_MODE, false, "Invalid addressing mode '%s'.")
X(ERROR_ILLEGAL_FORCED_ADDRESSING_MODE, true,
    "Invalid forced addressing mode on '%s'.")
X(ERROR_NOT_ENOUGH_ARGUMENTS_PASSED_TO_MACRO, true,
    "Not enough arguments passed to macro.")
X(ERROR_PREMATURE_EOF, false, "Premature end of file.")
X(ERROR_ILLEGAL_CHARACTER, true, "Invalid character '%s'.")
X(ERROR_BRANCH_OUT_OF_RANGE, true, "Branch out of range (%s bytes).")
X(ERROR_ERR_PSEUDO_OP_ENCOUNTERED, true, "ERR pseudo-op encountered.")
X(ERROR_ORIGIN_REVERSE_INDEXED, false, "Origin Reverse-indexed.")
X(ERROR_EQU_VALUE_MISMATCH, false, "EQU: Value mismatch.")
X(ERROR_ADDRESS_MUST_BE_LT_100, true, "Value in '%s' must be <$100.")
X(ERROR_ILLEGAL_BIT_SPECIFICATION, true, "Illegal bit specification.")
X(ERROR_NOT_ENOUGH_ARGS, true, "Not enough arguments.")
X(ERROR_LABEL_MISMATCH, true, "Label mismatch...\n --> %s")
X(ERROR_VALUE_UNDEFINED, true, "Value Undefined.")
X(ERROR_PROCESSOR_NOT_SUPPORTED, true, "Processor '%s' not supported.")
X(ERROR_REPEAT_NEGATIVE, false, "REPEAT parameter < 0 (ignored).")
X(ERROR_BADERROR, true, "Bad error value (internal error).")
X(ERROR_ONLY_ONE_PROCESSOR_SUPPORTED, true,
    "Only one processor type may be selected.")
X(ERROR_BAD_FORMAT, true, "Bad output format specified.")

/* new for genfill() check? TODO: refactor others to this one?[phf] */
X(ERROR_INVALID_RANGE, true, "Invalid range, %s.")

/* new generic errors when we don't want to define explicit ones [phf] */
X(ERROR_GENERIC_DEBUG, false, "%s (generic).")
X(ERROR_GENERIC_INFO, false, "%s (generic).")
X(ERROR_GENERIC_NOTICE, false, "%s (generic).")
X(ERROR_GENERIC_WARNING, false, "%s (generic).")
X(ERROR_GENERIC_ERROR, false, "%s (generic).")
X(ERROR_GENERIC_FATAL, true, "%s (generic).")
X(ERROR_GENERIC_PANIC, true, "%s (generic).")

/* F8 errors */
X(ERROR_VALUE_MUST_BE_1_OR_4, true, "Value in '%s' must be 1 or 4.")
X(ERROR_VALUE_MUST_BE_LT_10, true, "Value in '%s' must be <$10.")
X(ERROR_VALUE_MUST_BE_LT_8, true, "Value in '%s' must be <$8.")
X(ERROR_VALUE_MUST_BE_LT_F, true, "Value in '%s' must be <$f.")
X(ERROR_VALUE_MUST_BE_LT_10000, true, "Value in '%s' must be <$10000.")
X(ERROR_ILLEGAL_OPERAND_COMBINATION, true,
    "Invalid combination of operands '%s'")

/* marker for end of error codes, always keep this last! */
X(ERROR_MAX, true, "Invalid error code, marks end of table, report the bug!")
/*
    note that we could use a Y macro for ERROR_MAX to remove trailing
    comma, especially when we define the array of error messages where
    it will lead to an additional NULLed-out entry; but we only waste
    one small entry anyway... [phf]
*/

/* vim: set syntax=c tabstop=4 softtabstop=4 expandtab shiftwidth=4 autoindent: */
