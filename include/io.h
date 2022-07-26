#ifndef LIX_IO_H
#define LIX_IO_H


#include <lval.h>


//////////////////////
/// `lval` Reading ///
//////////////////////


///////////////////////
/// `lval` Printing ///
///////////////////////

/// \brief Prints the lval
///
/// \details Prints the lval to stdout.
/// 
/// \param v - type: lval* 
void lval_print(lval* v);


/// \brief Prints the lval to stdout with a newline.
///
/// \param v - type: lval*
void lval_println(lval* v);


///////////////////////////
/// Expression Printing ///
///////////////////////////

/// \brief Prints the lval as an expression to stdout.
///
/// \details Prints the lval as an expression surrounded
/// by the `open` and `close` parameters.
///
/// \param v - type: lval*
/// \param open - type: char
/// \param close - type: char
void lval_expr_print(lval* v, char open, char close);


/////////////////
/// String IO ///
/////////////////

/// TODO
void lval_print_str(lval* v);



#endif  /// LIX_IO_H
