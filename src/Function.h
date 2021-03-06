#ifndef HALIDE_FUNCTION_H
#define HALIDE_FUNCTION_H

/** \file
 * Defines the internal representation of a halide function and related classes
 */

#include "Expr.h"
#include "IntrusivePtr.h"
#include "Parameter.h"
#include "Schedule.h"
#include "Reduction.h"

namespace Halide {

namespace Internal {
struct FunctionContents;
}

/** An argument to an extern-defined Func. May be a Function, Buffer,
 * ImageParam or Expr. */
struct ExternFuncArgument {
    enum ArgType {UndefinedArg = 0, FuncArg, BufferArg, ExprArg, ImageParamArg};
    ArgType arg_type;
    Internal::IntrusivePtr<Internal::FunctionContents> func;
    Buffer buffer;
    Expr expr;
    Internal::Parameter image_param;

    ExternFuncArgument(Internal::IntrusivePtr<Internal::FunctionContents> f): arg_type(FuncArg), func(f) {}

    ExternFuncArgument(Buffer b): arg_type(BufferArg), buffer(b) {}

    ExternFuncArgument(Expr e): arg_type(ExprArg), expr(e) {}
    ExternFuncArgument(int e): arg_type(ExprArg), expr(e) {}
    ExternFuncArgument(float e): arg_type(ExprArg), expr(e) {}

    ExternFuncArgument(Internal::Parameter p) : arg_type(ImageParamArg), image_param(p) {
        // Scalar params come in via the Expr constructor.
        internal_assert(p.is_buffer());
    }
    ExternFuncArgument() : arg_type(UndefinedArg) {}

    bool is_func() const {return arg_type == FuncArg;}
    bool is_expr() const {return arg_type == ExprArg;}
    bool is_buffer() const {return arg_type == BufferArg;}
    bool is_image_param() const {return arg_type == ImageParamArg;}
    bool defined() const {return arg_type != UndefinedArg;}
};

namespace Internal {

struct UpdateDefinition {
    std::vector<Expr> values, args;
    Schedule schedule;
    ReductionDomain domain;
};

/** A reference-counted handle to Halide's internal representation of
 * a function. Similar to a front-end Func object, but with no
 * syntactic sugar to help with definitions. */
class Function {
private:
    IntrusivePtr<FunctionContents> contents;
public:
    /** Construct a new function with no definitions and no name. This
     * constructor only exists so that you can make vectors of
     * functions, etc.
     */
    EXPORT Function();

    /** Reconstruct a Function from a FunctionContents pointer. */
    EXPORT Function(const IntrusivePtr<FunctionContents> &c) : contents(c) {}

    /** Construct a new function with the given name */
    EXPORT Function(const std::string &n);

    /** Add a pure definition to this function. It may not already
     * have a definition. All the free variables in 'value' must
     * appear in the args list. 'value' must not depend on any
     * reduction domain */
    EXPORT void define(const std::vector<std::string> &args, std::vector<Expr> values);

    /** Add an update definition to this function. It must already
     * have a pure definition but not an update definition, and the
     * length of args must match the length of args used in the pure
     * definition. 'value' must depend on some reduction domain, and
     * may contain variables from that domain as well as pure
     * variables. Any pure variables must also appear as Variables in
     * the args array, and they must have the same name as the pure
     * definition's argument in the same index. */
    EXPORT void define_update(const std::vector<Expr> &args, std::vector<Expr> values);

    /** Accept a visitor to visit all of the definitions and arguments
     * of this function. */
    EXPORT void accept(IRVisitor *visitor) const;

    /** Get the name of the function */
    EXPORT const std::string &name() const;

    /** Get the pure arguments */
    EXPORT const std::vector<std::string> &args() const;

    /** Get the dimensionality */
    int dimensions() const {
        return (int)args().size();
    }

    /** Get the number of outputs */
    int outputs() const {
        return (int)output_types().size();
    }

    /** Get the types of the outputs */
    EXPORT const std::vector<Type> &output_types() const;

    /** Get the right-hand-side of the pure definition */
    EXPORT const std::vector<Expr> &values() const;

    /** Does this function have a pure definition */
    bool has_pure_definition() const {
        return !values().empty();
    }

    /** Does this function *only* have a pure definition */
    bool is_pure() const {
        return (has_pure_definition() &&
                !has_update_definition() &&
                !has_extern_definition());
    }

    /** Get a handle to the schedule for the purpose of modifying
     * it */
    EXPORT Schedule &schedule();

    /** Get a const handle to the schedule for inspecting it */
    EXPORT const Schedule &schedule() const;

    /** Get a handle on the output buffer used for setting constraints
     * on it. */
    EXPORT const std::vector<Parameter> &output_buffers() const;

    /** Get a mutable handle to the schedule for the update
     * stage */
    EXPORT Schedule &update_schedule(int idx = 0);

    /** Get a const reference to this function's update definitions. */
    EXPORT const std::vector<UpdateDefinition> &updates() const;

    /** Does this function have an update definition */
    EXPORT bool has_update_definition() const;

    /** Check if the function has an extern definition */
    EXPORT bool has_extern_definition() const;

    /** Add an external definition of this Func */
    EXPORT void define_extern(const std::string &function_name,
                              const std::vector<ExternFuncArgument> &args,
                              const std::vector<Type> &types,
                              int dimensionality);

    /** Retrive the arguments of the extern definition */
    EXPORT const std::vector<ExternFuncArgument> &extern_arguments() const;

    /** Get the name of the extern function called for an extern
     * definition. */
    EXPORT const std::string &extern_function_name() const;

    /** Equality of identity */
    bool same_as(const Function &other) const {
        return contents.same_as(other.contents);
    }

    /** Get a const handle to the debug filename */
    EXPORT const std::string &debug_file() const;

    /** Get a handle to the debug filename */
    EXPORT std::string &debug_file();

    /** Use an an extern argument to another function. */
    operator ExternFuncArgument() const {
        return ExternFuncArgument(contents);
    }

    /** Tracing calls and accessors, passed down from the Func
     * equivalents. */
    // @{
    EXPORT void trace_loads();
    EXPORT void trace_stores();
    EXPORT void trace_realizations();
    EXPORT bool is_tracing_loads() const;
    EXPORT bool is_tracing_stores() const;
    EXPORT bool is_tracing_realizations() const;
    // @}

    /** Mark function as frozen, which means it cannot accept new
     * definitions. */
    EXPORT void freeze();

    /** Check if a function has been frozen. If so, it is an error to
     * add new definitions. */
    EXPORT bool frozen() const;
};

}}

#endif
