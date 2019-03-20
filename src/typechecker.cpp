// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; -*-

/* libutap - Uppaal Timed Automata Parser.
   Copyright (C) 2002-2004 Uppsala University and Aalborg University.
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA
*/

#include <cmath>
#include <cstdio>
#include <cassert>
#include <list>

#include "utap/utap.h"
#include "utap/typechecker.h"
#include "utap/systembuilder.h"

using std::exception;
using std::set;
using std::pair;
using std::make_pair;
using std::max;
using std::min;
using std::map;
using std::vector;
using std::list;

using namespace UTAP;
using namespace Constants;

static bool isCost(expression_t expr) 
{
    return expr.getType().getBase() == type_t::COST;
}

static bool isVoid(expression_t expr) 
{
    return expr.getType().isVoid();
}

static bool isScalar(expression_t expr) 
{
    return expr.getType().isScalar();
}

static bool isInteger(expression_t expr) 
{
    return expr.getType().isInteger();
}

static bool isValue(expression_t expr) 
{
    return expr.getType().isValue();
}

static bool isClock(expression_t expr) 
{
    return expr.getType().isClock();
}

static bool isRecord(expression_t expr)
{
    return expr.getType().isRecord();
}

static bool isDiff(expression_t expr) 
{
    return expr.getType().isDiff();
}

static bool isInvariant(expression_t expr) 
{
    return expr.getType().isInvariant();
}

/**
   Returns true iff type is a valid invariant. A valid invariant is
   either an invariant expression or an integer expression.
*/
static bool isInvariantWR(expression_t expr) 
{
    return isInvariant(expr) || (expr.getType() == type_t::INVARIANT_WR);
}

/**
   Returns true iff type is a valid guard. A valid guard is either a
   valid invariant or a guard expression.
*/
static bool isGuard(expression_t expr)
{
    return expr.getType().isGuard();
}

static bool isConstraint(expression_t expr) 
{
    return expr.getType().isConstraint();
}

class InitialiserException : public std::exception
{
private:
    expression_t expr;
    char msg[256];
public:
    InitialiserException(expression_t expr, const char *m):
	expr(expr) { strncpy(msg, m, 256); }
    ~InitialiserException() throw() {}
    expression_t getExpression() { return expr; }
    const char *what() const throw() { return msg; }
};

void PersistentVariables::visitVariable(variable_t &variable)
{
    if (!variable.uid.getType().hasPrefix(prefix::CONSTANT))
    {
	variables.insert(variable.uid);
    }
}

void PersistentVariables::visitTemplateAfter(template_t &temp)
{
    SystemVisitor::visitTemplateAfter(temp);

    frame_t parameters = temp.parameters;
    for (uint32_t i = 0; i < parameters.getSize(); i++) 
    {
	if (parameters[i].getType().hasPrefix(prefix::REFERENCE)
	    || !parameters[i].getType().hasPrefix(prefix::CONSTANT))
	{
	    variables.insert(parameters[i]);
	}
    }
}

const set<symbol_t> &PersistentVariables::get() const
{
    return variables;
}

TypeChecker::TypeChecker(TimedAutomataSystem *system, ErrorHandler *handler)
    : ContextVisitor(handler), system(system)
{
    system->accept(persistentVariables);

    annotate(system->getBeforeUpdate());
    annotate(system->getAfterUpdate());
}

/** 
 * Annotate the expression and check that it is a constant
 * integer. Returns true iff no error were found.
 */
bool TypeChecker::annotateAndExpectConstantInteger(expression_t expr)
{
    if (annotate(expr))
    {
	if (!isInteger(expr))
	{
	    handleError(expr, "Integer expression expected");
	}
	else if (expr.dependsOn(persistentVariables.get()))
	{
	    handleError(expr, "Constant expression expected");
	}
	else
	{
	    return true;
	}
    }
    return false;
}

/** Check that the type is type correct (i.e. all expression such
 *  as array sizes, integer ranges, etc. contained in the type).
 */
void TypeChecker::checkType(type_t type, bool inRecord)
{
    type_t base = type.getBase();
    if (base == type_t::INT || base == type_t::SCALAR)
    {
	/* We can handle integer/scalar definitions with zero
	 * (constants), one (typically scalarsets) or two (typically
	 * integers) range expressions.
	 */
	expression_t l = type.getRange().first;
	expression_t u = type.getRange().second;

	if (!l.empty())
	{
	    /* Bounds must be constant integers.
	     */
	    if (annotateAndExpectConstantInteger(l) &&
		annotateAndExpectConstantInteger(u)) 
	    {
		/* Check if this is a valid range, i.e. the lower
		 * range is lower than or equal to the upper range. In
		 * fact, equality would also be rather useless, but it
		 * is semantically well defined.
		 *
		 * Errors evaluating the range expressions are ignored
		 * since either they are not well-typed (and thus an
		 * error was generated above) or they depend on
		 * template parameters, which we ignore (except when
		 * defined inside a record).
		 */
		try 
		{
		    Interpreter interpreter(system->getConstantValuation());
		    int32_t lower = interpreter.evaluate(l);
		    try 
		    {
			int32_t upper = interpreter.evaluate(u);
			if (lower > upper) 
			{
			    handleError(u, "Invalid integer range");
			}
		    }
		    catch (InterpreterException) 
		    {
			if (inRecord)
			{
			    /* REVISIT: In case the exception was
			     * generated by other problems, these
			     * error messages are spurious.
			     */
			    handleError(u, "Parameterised types not allowed in records");			
			}
		    }
		}
		catch (InterpreterException) 
		{
		    if (inRecord)
		    {
			/* REVISIT: In case the exception was
			 * generated by other problems, these error
			 * messages are spurious.
			 */
			handleError(l, "Parameterised types not allowed in records");
		    }

		}
	    }
	}
    }
    else if (base == type_t::ARRAY)
    {
	type_t size = type.getArraySize();
	checkType(size);
	checkType(type.getSub(), inRecord);
	if (!size.isScalar())
	{
	    /* The position is given by the upper bound of the
	     * range. See encoding in SystemBuilder.
	     */
	    handleError(size.getRange().second, "Invalid array size");
	}
	else
	{
	    try 
	    {
		Interpreter interpreter(system->getConstantValuation());
		int32_t lower = interpreter.evaluate(size.getRange().first);
		int32_t upper = interpreter.evaluate(size.getRange().second);
		if (lower > upper) 
		{
		    handleError(size.getRange().second, "Invalid array size");
		}
	    } 
	    catch (InterpreterException) 
	    {
		/* Array dimension is not computable, i.e. it depends
		 * on a parameter. TODO: We should check it for the
		 * instances!
		 */
		if (inRecord)
		{
		    /* REVISIT: In case the exception was generated by
		     * other problems, these error messages are
		     * spurious.
		     */
		    handleError(size.getRange().second,
				"Parameterised types not allowed in records");
		}
	    }
	}
    }
    else if (base == type_t::RECORD)
    {
	frame_t frame = type.getFrame();
	for (size_t i = 0; i < frame.getSize(); i++)
	{
	    checkType(frame[i].getType(), true);
	}
    }
}

void TypeChecker::checkVariableDeclaration(variable_t &variable)
{
    setContextDeclaration();
    checkType(variable.uid.getType());
    checkInitialiser(variable);
}

bool TypeChecker::visitTemplateBefore(template_t &temp)
{
    ContextVisitor::visitTemplateBefore(temp);

    setContextParameters();
    frame_t parameters = temp.parameters;
    for (size_t i = 0; i < parameters.getSize(); i++) 
    {
	checkType(parameters[i].getType());
    }
    return true;
}

void TypeChecker::visitVariable(variable_t &variable)
{
    ContextVisitor::visitVariable(variable);

    checkVariableDeclaration(variable);
    if (variable.uid.getType().hasPrefix(prefix::CONSTANT))
    {
	system->getConstantValuation()[variable.uid] = variable.expr;
    }
}

class RateDecomposer
{
public:
    list<pair<expression_t,expression_t> > rate;
    expression_t invariant;

    void decompose(expression_t);
};

void RateDecomposer::decompose(expression_t expr)
{
    assert(isInvariantWR(expr));
    assert(isInvariant(expr) || expr.getKind() == AND || expr.getKind() == EQ);

    if (isInvariant(expr))
    {
	if (invariant.empty())
	{
	    invariant = expr;
	}
	else
	{
	    invariant = expression_t::createBinary(
		position_t(), AND, invariant, expr);
	}	    
    }
    else if (expr.getKind() == AND)
    {
	decompose(expr[0]);
	decompose(expr[1]);
    }
    else
    {
	assert(expr[0].getType() == type_t::RATE
	       ^ expr[1].getType() == type_t::RATE);
	
	if (expr[0].getType() == type_t::RATE)
	{
	    rate.push_back(make_pair(expr[0][0], expr[1]));
	}
	else
	{
	    rate.push_back(make_pair(expr[1][0], expr[0]));
	}
    }
}


void TypeChecker::visitState(state_t &state)
{
    ContextVisitor::visitState(state);

    if (!state.invariant.empty()) 
    {
	setContextInvariant(state);

	if (annotate(state.invariant))
	{    
	    if (!isInvariantWR(state.invariant))
	    {
		handleError(state.invariant, "Invalid invariant expression");
	    }
	    if (!isSideEffectFree(state.invariant))
	    {
		handleError(state.invariant, "Invariant must be side effect free");
	    }
	}

	RateDecomposer decomposer;
	decomposer.decompose(state.invariant);
	state.invariant = decomposer.invariant;
	if (!decomposer.rate.empty())
	{
	    state.costrate = decomposer.rate.front().second;
	}
    }
}

void TypeChecker::visitEdge(edge_t &edge)
{
    ContextVisitor::visitEdge(edge);

    // select
    setContextSelect(edge);
    frame_t select = edge.select;
    for (size_t i = 0; i < select.getSize(); i++)
    {
	checkType(select[i].getType());
    }
    
    // guard
    setContextGuard(edge);
    if (!edge.guard.empty())
    {
	if (annotate(edge.guard))
	{    
	    if (!isGuard(edge.guard))
	    {
		handleError(edge.guard, "Invalid guard");
	    }
	    else if (!isSideEffectFree(edge.guard))
	    {
		handleError(edge.guard, "Guard must be side effect free");
	    }
	}
    }

    // sync
    if (!edge.sync.empty()) 
    {
	setContextSync(edge);
	if (annotate(edge.sync))
	{
	    type_t channel = edge.sync.get(0).getType();
	    if (channel.getBase() != type_t::CHANNEL)
	    {
		handleError(edge.sync.get(0), "Channel expected");
	    } 
	    else if (!isSideEffectFree(edge.sync))
	    {
		handleError(edge.sync,
			    "Synchronisation must be side effect free");
	    }
	    else
	    {
		bool hasClockGuard =
		    !edge.guard.empty() && !isValue(edge.guard);
		bool isUrgent = channel.hasPrefix(prefix::URGENT);
		bool receivesBroadcast = channel.hasPrefix(prefix::BROADCAST) 
		    && edge.sync.getSync() == SYNC_QUE;
		
		if (isUrgent && hasClockGuard) 
		{
		    handleError(edge.sync,
				"Clock guards are not allowed on urgent edges");
		} 
		else if (receivesBroadcast && hasClockGuard) 
		{
		    handleError(edge.sync,
				"Clock guards are not allowed on broadcast receivers");
		}
	    }
	}
    }
    
    // assignment
    setContextAssignment(edge);
    if (annotate(edge.assign))
    {
	if (!isValue(edge.assign)
	    && !isScalar(edge.assign)
	    && !isClock(edge.assign)
	    && !isRecord(edge.assign)
	    && !isCost(edge.assign)    
	    && !isVoid(edge.assign))
	{
	    handleError(edge.assign, "Invalid assignment expression");
	}
	
	if (!(edge.assign.getKind() == CONSTANT &&
	      edge.assign.getValue() == 1)
	    && isSideEffectFree(edge.assign))
	{
	    handleWarning(edge.assign, "Expression does not have any effect");
	}
    }
    setContextNone();
}

void TypeChecker::visitProgressMeasure(progress_t &progress)
{
    annotate(progress.guard);
    annotate(progress.measure);

    if (!progress.guard.empty() && !isValue(progress.guard))
    {
	handleError(progress.guard, 
		    "Progress measure must evaluate to a boolean");
    }

    if (!isValue(progress.measure))
    {
	handleError(progress.measure,
		    "Progress measure must evaluate to a value");
    }
}

void TypeChecker::visitInstance(instance_t &instance)
{
    ContextVisitor::visitInstance(instance);

    Interpreter interpreter(system->getConstantValuation());
    interpreter.addValuation(instance.mapping);

    setContextInstantiation();

    map<symbol_t, expression_t>::iterator i = instance.mapping.begin();
    for (;i != instance.mapping.end(); ++i) 
    {
	type_t parameter = i->first.getType();
	expression_t argument = i->second;
	
	if (!annotate(argument))
	{
	    continue;
	}

	// For template instantiation, the argument must be side effect free
	if (!isSideEffectFree(argument)) 
	{
	    handleError(argument, "Argument must be side effect free");
	    continue;
	}

	// We have three ok cases:
	// - Constant reference with computable argument
	// - Reference parameter with unique lhs argument
	// - Value parameter with computable argument
	// If non of the cases is true, then we generate an error
	bool ref = parameter.hasPrefix(prefix::REFERENCE);
	bool constant = parameter.hasPrefix(prefix::CONSTANT);
	bool computable = !argument.dependsOn(persistentVariables.get());
	
	if (!(ref && constant && computable)
	    && !(ref ? isUniqueReference(argument) : computable))
	{
	    handleError(argument, "Incompatible argument");
	    continue;
	}

	checkParameterCompatible(interpreter, parameter, argument);
    }
}

void TypeChecker::visitProperty(expression_t expr)
{
    setContextNone();
    if (annotate(expr))
    {
	if (!isSideEffectFree(expr)) 
	{
	    handleError(expr, "Property must be side effect free");
	}
	
	if ((expr.getKind() == LEADSTO &&
	     !(isConstraint(expr[0]) && isConstraint(expr[1])))
	    || (expr.getKind() != LEADSTO && !isConstraint(expr[0])))
	{
	    handleError(expr, "Property must be a constraint");
	}
    }
}

void TypeChecker::checkAssignmentExpressionInFunction(expression_t expr)
{
    if (!isValue(expr) && !isClock(expr) && !isRecord(expr) && !isVoid(expr)
	&& !isScalar(expr))
    {
	handleError(expr, "Invalid expression in function");
    }

//     if (isSideEffectFree(expr)) {
//  	handleWarning(expr, "Expression does not have any effect");
//     }
}

void TypeChecker::checkConditionalExpressionInFunction(expression_t expr)
{
    if (!isValue(expr)) 
    {
	handleError(expr, "Boolean expected");
    }
}

void TypeChecker::visitFunction(function_t &fun)
{
    ContextVisitor::visitFunction(fun);

    setContextDeclaration();

    // Type check the function body
    fun.body->accept(this);

    // Collect symbols which are changed by the function
    CollectChangesVisitor visitor(fun.changes);
    fun.body->accept(&visitor);    

    CollectDependenciesVisitor visitor2(fun.depends);
    fun.body->accept(&visitor2);

    setContextNone();
    // TODO: Make sure that last statement is a return statement
}

int32_t TypeChecker::visitEmptyStatement(EmptyStatement *stat)
{
    return 0;
}

int32_t TypeChecker::visitExprStatement(ExprStatement *stat)
{
    if (annotate(stat->expr))
    {
	checkAssignmentExpressionInFunction(stat->expr);
    }
    return 0;
}

int32_t TypeChecker::visitForStatement(ForStatement *stat)
{
    if (annotate(stat->init))
    {
	checkAssignmentExpressionInFunction(stat->init);
    }

    if (annotate(stat->cond))
    {
	checkConditionalExpressionInFunction(stat->cond);
    }

    if (annotate(stat->step))
    {
	checkAssignmentExpressionInFunction(stat->step);
    }

    return stat->stat->accept(this);
}

int32_t TypeChecker::visitIterationStatement(IterationStatement *stat)
{
    checkType(stat->symbol.getType(), false);
    return stat->stat->accept(this);
}

int32_t TypeChecker::visitWhileStatement(WhileStatement *stat)
{
    if (annotate(stat->cond))
    {
	checkConditionalExpressionInFunction(stat->cond);
    }
    return stat->stat->accept(this);
}

int32_t TypeChecker::visitDoWhileStatement(DoWhileStatement *stat)
{
    if (annotate(stat->cond))
    {
	checkConditionalExpressionInFunction(stat->cond);
    }
    return stat->stat->accept(this);
}

int32_t TypeChecker::visitBlockStatement(BlockStatement *stat)
{
    BlockStatement::iterator i;

    /* Check type and initialiser of local variables (parameters are
     * also considered local variables).
     */
    frame_t frame = stat->getFrame();
    for (uint32_t i = 0; i < frame.getSize(); i++)
    {
	symbol_t symbol = frame[i];
	checkType(symbol.getType());
	if (symbol.getData())
	{
	    variable_t *var = static_cast<variable_t*>(symbol.getData());
	    if (!var->expr.empty())
	    {
		annotate(var->expr);
		var->expr = checkInitialiser(symbol.getType(), var->expr);
	    }
	}	
    }    

    /* Check statements.
     */
    for (i = stat->begin(); i != stat->end(); ++i) 
    {
	(*i)->accept(this);
    }
    return 0;
}

int32_t TypeChecker::visitSwitchStatement(SwitchStatement *stat)
{
    annotate(stat->cond);
    // TODO: Check type of expressions
    return visitBlockStatement(stat);
}

int32_t TypeChecker::visitCaseStatement(CaseStatement *stat)
{
    annotate(stat->cond);
    // TODO: Check type of expressions
    return visitBlockStatement(stat);
}

int32_t TypeChecker::visitDefaultStatement(DefaultStatement *stat)
{
    return visitBlockStatement(stat);
}

int32_t TypeChecker::visitIfStatement(IfStatement *stat)
{
    if (annotate(stat->cond))
    {
	checkConditionalExpressionInFunction(stat->cond);
    }
    stat->trueCase->accept(this);
    if (stat->falseCase) 
    {
	stat->falseCase->accept(this);
    }
    return 0;
}

int32_t TypeChecker::visitBreakStatement(BreakStatement *stat)
{
    return 0;
}

int32_t TypeChecker::visitContinueStatement(ContinueStatement *stat)
{   
    return 0;
}

int32_t TypeChecker::visitReturnStatement(ReturnStatement *stat)
{
    annotate(stat->value);
    // TODO: Check type of expressions
    return 0;
}

/** Returns a value indicating the capabilities of a channel. For
    urgent channels this is 0, for non-urgent broadcast channels this
    is 1, and in all other cases 2. An argument to a channel parameter
    must have at least the same capability as the parameter. */
static int channelCapability(type_t type)
{
    assert(type.getBase() == type_t::CHANNEL);
    if (type.hasPrefix(prefix::URGENT))
    {
	return 0;
    }
    if (type.hasPrefix(prefix::BROADCAST))
    {
	return 1;
    }
    return 2;
}

/**
 * Checks whether argument type is compatible with parameter type.
 *
 * REVISIT: The reasoning behind the current implementation is
 * strange. For constant reference parameters, it is ok to specify
 * constant arguments; but these arguments might themself be constant
 * references to non-constant variables. E.g.
 *
 *   void f(const int &i) {}
 *   void g(const int &j) { f(j); }
 *
 * where g() is called with a regular variable. When checking the call
 * of f() in g(), we have that isLHSValue(j) return false (because we
 * cannot assign to j in g()). We then conclude that the call is valid
 * anyway (which is a correct conclusion), because we can always
 * evaluate j and create a temporary variable for i (this is an
 * incorrect argument, because what actually happens is that we pass
 * on the reference we got when g() was called).
 *
 * The current implementation seems to work, but for the wrong
 * reasons!
 */
void TypeChecker::checkParameterCompatible(
    const Interpreter &interpreter, type_t paramType, expression_t arg)
{
    try 
    {
	bool ref = paramType.hasPrefix(prefix::REFERENCE);
	bool constant = paramType.hasPrefix(prefix::CONSTANT);
	bool lhs = isLHSValue(arg);
	
	type_t argType = arg.getType();
	
	if (!ref) 
	{
	    // If the parameter is not a reference, then we can do type
	    // conversion between booleans and integers.
	    
	    if (paramType.getBase() == type_t::INT
		&& argType.getBase() == type_t::BOOL)
	    {
		argType = type_t::createInteger(
		    expression_t::createConstant(position_t(), 0),
		    expression_t::createConstant(position_t(), 1));
		lhs = false;
	    }
	    
	    if (paramType.getBase() == type_t::BOOL
		&& argType.getBase() == type_t::INT)
	    {
		argType = type_t::BOOL;
		lhs = false;
	    }
	}
	
	// For non-const reference parameters, we require a lhs argument
	if (ref && !constant && !lhs)
	{
	    throw "Reference parameter requires left value argument";
	}
	
	// Resolve base type of arrays
	while (paramType.getBase() == type_t::ARRAY) 
	{
	    if (argType.getBase() != type_t::ARRAY) 
	    {
		throw "Incompatible type";
	    }
	    
	    type_t argSize = argType.getArraySize();
	    type_t paramSize = paramType.getArraySize();
		
	    if (argSize.isInteger() && paramSize.isInteger())
	    {
		/* Here we enforce that the expression used to declare
		 * the size of the parameter and argument arrays are
		 * equal: This is more restrictive that it needs to
		 * be. However, evaluating the size is not always
		 * possible (e.g. for function calls), hence we resort
		 * to this strict check.
		 */
		if (!argSize.getRange().first.equal(paramSize.getRange().first)
		    || !argSize.getRange().second.equal(paramSize.getRange().second))
		{
		    throw "Incompatible type";
		}
	    }
	    else if (argSize.isScalar() && paramSize.isScalar())
	    {
		if (argSize != paramSize)
		{
		    throw "Incompatible type";
		}		    
	    }
	    else
	    {
		/* This should be unreachable - if not, then we have
		 * somehow used a type which is not either an integer
		 * or scalarset.
		 */
		throw "Incompatible type";
	    }

	    paramType = paramType.getSub();
	    argType = argType.getSub();
	}
	
	// The parameter and the argument must have the same base type
	if (paramType.getBase() != argType.getBase()) 
	{
	    throw "Incompatible argument";
	}
	
	type_t base = paramType.getBase();
	if (base == type_t::CLOCK || base == type_t::BOOL) 
	{
	    // For clocks and booleans there is no more to check
	    return;
	}
	
	if (base == type_t::INT) 
	{
	    // For integers we need to consider the range: The main
	    // purpose is to ensure that arguments to reference parameters
	    // are within range of the parameter. For non-reference
	    // parameters we still try to check whether the argument is
	    // outside the range of the parameter, but this can only be
	    // done if the argument is computable at parse time.
	    
	    // Special case; if parameter has no range, then everything
	    // is accepted - this ensures compatibility with 3.2
	    if (paramType.getRange().first.empty())
	    {
		return;
	    }
	    
	    // There are two main cases
	    //
	    // case a: if we have a left value argument, then there is no
	    // way we can compute the exact value of the argument. In this
	    // case we must use the declared range.
	    //
	    // case b: if it is not a left value argument, then we might
	    // be able to compute the exact value, which is what we will
	    // try to do.
	    
	    if (lhs) 
	    {
		// case a
		try 
		{
		    // First try to compute the declared range of the
		    // argument and the paramter.
		    range_t paramRange = interpreter.evaluate(paramType.getRange());
		    range_t argRange = interpreter.evaluate(argType.getRange());
		    
		    // For non-constant reference parameters the argument
		    // range must match that of the parameter.
		    if (ref && !constant && argRange != paramRange) 
		    {
			throw "Range of argument does not match range of formal parameter";
		    }
		    
		    // For constant reference parameters the argument
		    // range must be contained in the paramtere range.
		    if (ref && constant && !paramRange.contains(argRange)) 
		    {
			throw "Range of argument is outside of the range of the formal parameter";
		    }
		    
		    // In case the two ranges do not intersect at all,
		    // then the argument can never be valid.
		    if (paramRange.intersect(argRange).isEmpty()) 
		    {
			throw "Range of argument is outside of the range of the formal parameter";
		    }
		} 
		catch (InterpreterException) 
		{
		    // Computing the declared range failed.
		    
		    if (ref) 
		    {
			// For reference parameters we check that the
			// range declaration of the argument is identical
			// to that of the parameter.
			pair<expression_t, expression_t> paramRange, argRange;
			paramRange = paramType.getRange();
			argRange = argType.getRange();
			if (!paramRange.first.equal(argRange.first) ||
			    !paramRange.second.equal(argRange.second))
			{
			    throw "Range of argument does not match range of formal parameter";
			}
		    }
		}
	    }
	    else 
	    {
		// case b
		try 
		{
		    range_t argRange, paramRange;
		    
		    paramRange = interpreter.evaluate(paramType.getRange());
		    
		    vector<int32_t> value;
		    interpreter.evaluate(arg, value);
		    for (uint32_t i = 0; i < value.size(); i++) 
		    {
			argRange = argRange.join(range_t(value[i]));
		    }
		    
		    if (!paramRange.contains(argRange)) 
		    {
			throw "Range of argument is outside of the range of the formal parameter";
		    }
		} 
		catch (InterpreterException) 
		{
		    // Bad luck: we need to revert to runtime checking 
		}
	    }
	} 
	else if (base == type_t::RECORD) 
	{
	    if (paramType.getRecordFields() != argType.getRecordFields()) 
	    {
		throw "Argument has incompatible type";
	    }
	} 
	else if (base == type_t::CHANNEL) 
	{
	    if (channelCapability(argType) < channelCapability(paramType))
	    {
		throw "Incompatible channel type";
	    }
	} 
	else if (base == type_t::SCALAR)
	{
	    // At the moment we do not allow integer arguments of scalar
	    // parameters. We could allow this for the same reason as we
	    // could allow initialisers for scalars. However the compiler
	    // and symmetry filter need to be able to handle
	    // this. REVISIT.
	    if (paramType != argType) 
	    {
		throw "Argument has incompatible type";
	    }
	}
	else 
	{
	    assert(false);
	}
    }
    catch (const char *error)
    {
	handleError(arg, error);
    }
}

/** 
 * Checks whether init is a valid initialiser for a variable or
 * constant of the given type. If not an InitialiserException is
 * thrown. For record types, the initialiser is reordered to fit the
 * order of the fields and the new initialiser is returned.
 */
expression_t TypeChecker::checkInitialiser(type_t type, expression_t init)
{
    Interpreter interpreter(system->getConstantValuation());
    type_t base = type.getBase();
    if (base == type_t::ARRAY) 
    {
        if (init.getKind() != LIST)
	{
            throw InitialiserException(init, "Invalid array initialiser");
	}
	
	int32_t dim;
	try
	{
	    type_t size = type.getArraySize();
	    if (!size.isInteger())
	    {
		throw InitialiserException(
		    init, "Arrays of scalarsets cannot have initialisers");
	    }
	    int32_t lower = interpreter.evaluate(size.getRange().first);
	    int32_t upper = interpreter.evaluate(size.getRange().second);
	    dim = upper - lower + 1;
	}
	catch (InterpreterException) 
	{
	    throw InitialiserException(
		init, "Arrays with parameterized size cannot have an initialiser");
	}

        if (init.getSize() > (uint32_t)dim)
	{
            throw InitialiserException(init,
				       "Excess elements in array initialiser");
	}

	type_t subtype = type.getSub();
	frame_t fields = init.getType().getRecordFields();
	vector<expression_t> result(fields.getSize(), expression_t());
        for (uint32_t i = 0; i < fields.getSize(); i++) 
	{
            if (!fields[i].getName().empty()) 
	    {
		throw InitialiserException(
		    init[i], "Unknown field specified in initialiser");
	    }
            result[i] = checkInitialiser(subtype, init[i]);
        }
        
        if (fields.getSize() < (uint32_t)dim) 
	{
	    throw InitialiserException(init, "Missing fields in initialiser");
	}
	init = expression_t::createNary(
	    init.getPosition(), LIST, result, type);
    } 
    else if (base == type_t::BOOL) 
    {
	if (!isValue(init)) 
	{
	    throw InitialiserException(init, "Invalid initialiser");
	}
    } 
    else if (base == type_t::INT) 
    {
	if (!isValue(init)) 
	{
	    throw InitialiserException(init, "Invalid initialiser");
	}

	// If there is no range (this might be the case when the
	// variable is a constant), then we cannot do anymore.
	if (type.getRange().first.empty()) 
	{
	    return init;
	}

	// In general we cannot assure that the initialiser is within
	// the range of the variable - what we can do is to check that
	// if both the range of the variable and the initialiser are
	// computable, then the initialiser should be within the
	// range.

	try 
	{
	    // If possible, compute value and range
	    int n = interpreter.evaluate(init);
	    range_t range = interpreter.evaluate(type.getRange());
	
	    // YES! Everything was computable, so make sure that initialiser
	    // is within range.
	    if (!range.contains(n))
	    {
		throw InitialiserException(init, "Initialiser is out of range");
	    }
	} 
	catch (InterpreterException) 
	{
	    // NO! We cannot check more.
	}
    } 
    else if (base == type_t::RECORD) 
    {
	if (init.getType().getBase() == type_t::RECORD 
	    && type.getRecordFields() == init.getType().getRecordFields())
	{
	    return init;
	}

	if (init.getKind() != LIST) 
	{
	    throw InitialiserException(init, "Invalid initialiser for struct");
	}

	frame_t fields = type.getRecordFields();
	frame_t initialisers = init.getType().getRecordFields();
	vector<expression_t> result(fields.getSize(), expression_t());

	int32_t current = 0;
	for (uint32_t i = 0; i < initialisers.getSize(); i++, current++) 
	{
	    if (!initialisers[i].getName().empty())
	    {
		current = fields.getIndexOf(initialisers[i].getName());
		if (current == -1) 
		{
		    handleError(init[i], "Unknown field");
		    break;
		}
	    }

	    if (current >= (int32_t)fields.getSize()) 
	    {
		handleError(init[i], "Excess elements in intialiser");
		break;
	    }
	    
	    if (!result[current].empty()) 
	    {
		handleError(init[i], "Multiple initialisers for field");
		continue;
	    }
	    
	    result[current] = 
		checkInitialiser(fields[current].getType(), init[i]);
	}

	// Check that all fields do have an initialiser.
	for (uint32_t i = 0; i < fields.getSize(); i++) 
	{
	    if (result[i].empty()) 
	    {
		throw InitialiserException(init, "Incomplete initialiser");
	    }	
	}

	init = expression_t::createNary(
	    init.getPosition(), LIST, result, type);
    }
    else
    {
	throw InitialiserException(init, "Invalid initialiser");
    }
    return init;
}

/** Checks the initialiser of a constant or a variable */
void TypeChecker::checkInitialiser(variable_t &var)
{
    try 
    {
	if (!var.expr.empty() && annotate(var.expr))
	{
	    if (var.expr.dependsOn(persistentVariables.get())) 
	    {
		handleError(var.expr, "Constant expression expected");
	    }
	    else if (!isSideEffectFree(var.expr)) 
	    {
		handleError(var.expr, "Initialiser must not have side effects");
	    } 
	    else 
	    {
		var.expr = checkInitialiser(var.uid.getType(), var.expr);
	    }
	}
    } 
    catch (InitialiserException &e) 
    {
	handleError(e.getExpression(), e.what());
    }
}

/** Returns the type of a binary operation with non-integer operands. */
type_t TypeChecker::typeOfBinaryNonInt(
    expression_t left, uint32_t binaryop, expression_t right)
{
    type_t type;
    
    switch (binaryop) 
    {
    case PLUS:
	if (isInteger(left) && isClock(right)
	    || isClock(left) && isInteger(right))
	{
	    type = type_t::CLOCK;
	} 
	else if (isDiff(left) && isInteger(right) 
		 || isInteger(left) && isDiff(right))
	{
	    type = type_t::DIFF;
	}
	break;
	    
    case MINUS:
	if (isClock(left) && isInteger(right)) 
	    // removed  "|| isInteger(left.type) && isClock(right.type)" 
	    // in order to be able to convert into ClockGuards
	{
	    type = type_t::CLOCK;
	} 
	else if (isDiff(left) && isInteger(right)
		 || isInteger(left) && isDiff(right)
		 || isClock(left) && isClock(right)) 
	{
	    type = type_t::DIFF;
	}
	break;

    case AND:
	if (isInvariant(left) && isInvariant(right)) 
	{
	    type = type_t::INVARIANT;
	} 
	else if (isInvariantWR(left) && isInvariantWR(right)) 
	{
	    type = type_t::INVARIANT_WR;
	}
	else if (isGuard(left) && isGuard(right)) 
	{
	    type = type_t::GUARD;
	} 
	else if (isConstraint(left) && isConstraint(right)) 
	{
	    type = type_t::CONSTRAINT;
	}
	break;
	    
    case OR:
	if (isValue(left) && isInvariant(right))
	{
	    type = type_t::INVARIANT;
	}
	else if (isValue(left) && isGuard(right))
	{
	    type = type_t::GUARD;
	}
	else if (isConstraint(left) && isConstraint(right)) 
	{
	    type = type_t::CONSTRAINT;
	}
	break;

    case LT:
    case LE:
	if (isClock(left) && isClock(right)
	    || isClock(left) && isInteger(right)
	    || isDiff(left) && isInteger(right)
	    || isInteger(left) && isDiff(right))
	{
	    type = type_t::INVARIANT;
	} 
	else if (isInteger(left) && isClock(right)) 
	{
	    type = type_t::GUARD;
	}
	break;

    case EQ:
	if (isClock(left) && isClock(right)
	    || isClock(left) && isInteger(right)
	    || isInteger(left) && isClock(right)
	    || isDiff(left) && isInteger(right)
	    || isInteger(left) && isDiff(right))
	{
	    type = type_t::GUARD;
	}
	else if (left.getType() == type_t::RATE && isInteger(right)
		 || isInteger(left) && right.getType() == type_t::RATE)
	{
	    type = type_t::INVARIANT_WR;
	}
	break;
	
    case NEQ:
	if (isClock(left) && isClock(right)
	    || isClock(left) && isInteger(right)
	    || isInteger(left) && isClock(right)
	    || isDiff(left) && isInteger(right)
	    || isInteger(left) && isDiff(right))
	{
	    type = type_t::CONSTRAINT;
	}
	break;

    case GE:
    case GT:
	if (isClock(left) && isClock(right)
	    || isInteger(left) && isClock(right)
	    || isDiff(left) && isInteger(right)
	    || isInteger(left) && isDiff(right))
	{
	    type = type_t::INVARIANT;
	} 
	else if (isClock(left) && isGuard(right)) 
	{
	    type = type_t::GUARD;
	}
	break;
    }

    return type;
}

/** Returns true if arguments of an inline if are compatible. The
    'then' and 'else' arguments are compatible if and only if they
    have the same base type. In case of arrays, they must have the
    same size and the subtypes must be compatible. In case of records,
    they must have the same type name.
*/
bool TypeChecker::areInlineIfCompatible(type_t thenArg, type_t elseArg)
{
    type_t thenBase = thenArg.getBase();
    type_t elseBase = elseArg.getBase();

    if (thenArg.isValue() && elseArg.isValue())
    {
	return true;
    }
    else if (thenArg.isClock() && elseArg.isClock())
    {
	return true;
    }
    else if (thenBase == type_t::CHANNEL && elseBase == type_t::CHANNEL)
    {
	return (thenArg.hasPrefix(prefix::URGENT) 
		== elseArg.hasPrefix(prefix::URGENT))
	    && (thenArg.hasPrefix(prefix::BROADCAST) 
		== elseArg.hasPrefix(prefix::BROADCAST));
    }
    else if (thenBase == type_t::ARRAY && elseBase == type_t::ARRAY)
    {
	type_t thenSize = thenArg.getArraySize();
	type_t elseSize = elseArg.getArraySize();
	if (thenSize.isInteger() && elseSize.isInteger())
	{
	    if (!thenSize.getRange().first.equal(elseArg.getRange().first)
		|| !thenSize.getRange().second.equal(elseArg.getRange().second))
	    {
		return false;
	    }
	}
	else if (thenSize.isScalar() && elseSize.isScalar())
	{
	    if (thenSize != elseSize)
	    {
		return false;
	    }
	}
	else
	{
	    return false;
	}
	return areInlineIfCompatible(thenArg.getSub(), elseArg.getSub());
    }
    else if (thenArg.isRecord() && elseArg.isRecord())
    {
	return thenArg.getRecordFields() == elseArg.getRecordFields();
    }
    else if (thenArg.isScalar() && elseArg.isScalar())
    {
	return thenArg == elseArg;
    }	    

    return false;
}

/** Returns true if lvalue and rvalue are assignment compatible.  This
    is the case when an expression of type rvalue can be assigned to
    an expression of type lvalue. It does not check whether lvalue is
    actually a left-hand side value. In case of integers, it does not
    check the range of the expressions.
*/
bool TypeChecker::areAssignmentCompatible(type_t lvalue, type_t rvalue) 
{
    if (lvalue.isClock() && rvalue.isValue())
    {
	return true;
    }
    else if (lvalue.isValue() && rvalue.isValue())
    {
	return true;
    }
    else if (lvalue.isRecord() && rvalue.isRecord())
    {
	return lvalue.getRecordFields() == rvalue.getRecordFields();
    }
    else if (lvalue.isScalar() && rvalue.isScalar())
    {
	return lvalue == rvalue;
    }

    return false;
}

void TypeChecker::checkFunctionCallArguments(expression_t expr)
{
    // REVISIT: We don't know anything about the context of this
    // expression, but the additional mapping provided by the context
    // might be important additions to the interpreter. In particular,
    // it might be necessary to add the parameter mapping from the
    // call itself. E.g. consider a function
    //
    //  int f(const int N, int a[N])
    //
    // Here it is important to know N when checking the second
    // argument. At the moment this is not allowed by the
    // SystemBuilder, though.

    type_t type = expr[0].getType();
    frame_t parameters = type.getParameters();

    if (parameters.getSize() > expr.getSize() - 1) 
    {
	handleError(expr, "Too few arguments");
    }
    else if (parameters.getSize() < expr.getSize() - 1)
    {
	for (uint32_t i = parameters.getSize() + 1; i < expr.getSize(); i++)
	{
	    handleError(expr[i], "Too many arguments");
	}
    }
    else
    {
	Interpreter interpreter(system->getConstantValuation());
	for (uint32_t i = 0; i < parameters.getSize(); i++) 
	{
	    type_t parameter = parameters[i].getType();
	    expression_t argument = expr[i + 1];
	    checkParameterCompatible(interpreter, parameter, argument);
	}
    }
}

/** Type check and annotate the expression. This function performs
    basic type checking of the given expression and assigns a type to
    every subexpression of the expression. It checks that only
    left-hand side values are updated, checks that functions are
    called with the correct arguments, checks that operators are used
    with the correct operands and checks that operands to assignment
    operators are assignment compatible. Errors are reported by
    calling handleError(). This function does not check/compute the
    range of integer expressions and thus does not produce
    out-of-range errors or warnings. Returns true if no type errors
    were found, false otherwise.
*/
bool TypeChecker::annotate(expression_t expr)
{
    /* Do not annotate empty expressions.
     */
    if (expr.empty())
    {
	return true;
    }

    /* Annotate sub-expressions. 
     */
    bool ok = true;
    for (uint32_t i = 0; i < expr.getSize(); i++) 
    {
	ok &= annotate(expr[i]);
    }
    
    /* Do not annotate the expression if any of the sub-expressions
     * contained errors.
     */
    if (!ok)
    {
	return false;
    }

    /* Annotate the expression. This depends on the kind of expression
     * we are dealing with.
     */
    type_t type, arg1, arg2, arg3;
    switch (expr.getKind()) 
    {
    case EQ:
    case NEQ:
	if (isValue(expr[0]) && isValue(expr[1])) 
	{
	    type = type_t::BOOL;
	}
	else if (isRecord(expr[0]) && isRecord(expr[1])
		 && expr[0].getType().getRecordFields() 
		 == expr[1].getType().getRecordFields())
	{
	    type = type_t::BOOL;
	}
	else if (expr[0].getType().getBase() == type_t::SCALAR
		 || expr[1].getType().getBase() == type_t::SCALAR)
	{
	    if (expr[0].getType() != expr[1].getType())
	    {
		handleError(expr, "Scalars can only be compared to scalars of the same scalarset");
		return false;
	    }
	    type = type_t::BOOL;
	}
	else 
	{
	    type = typeOfBinaryNonInt(expr[0], expr.getKind(), expr[1]);
	    if (type == type_t()) 
	    {
		handleError(expr, "Invalid operands to binary operator");
		return false;
	    }
	}
	break;

    case PLUS:
    case MINUS:
    case MULT:
    case DIV:
    case MOD:
    case BIT_AND:
    case BIT_OR:
    case BIT_XOR:
    case BIT_LSHIFT:
    case BIT_RSHIFT:
    case MIN:
    case MAX:
	if (isValue(expr[0]) && isValue(expr[1]))
	{
	    type = type_t::INT;
	}
	else 
	{
	    type = typeOfBinaryNonInt(expr[0], expr.getKind(), expr[1]);
	    if (type == type_t()) 
	    {
		handleError(expr, "Invalid operands to binary operator");
		return false;
	    }
	}
	break;

    case AND:
    case OR:
    case LT:
    case LE:
    case GE:
    case GT:
	if (isValue(expr[0]) && isValue(expr[1]))
	{
	    type = type_t::BOOL;
	}
	else 
	{
	    type = typeOfBinaryNonInt(expr[0], expr.getKind(), expr[1]);
	    if (type == type_t()) 
	    {
		handleError(expr, "Invalid operands to binary operator");
		return false;
	    }
	}
	break;

    case NOT:
	if (isValue(expr[0])) 
	{
	    type = type_t::BOOL;
	}
	else if (isConstraint(expr[0])) 
	{
	    type = type_t::CONSTRAINT;
	}
	else 
	{
	    handleError(expr, "Invalid operation for type");
	    return false;
	}
	break;
	
    case UNARY_MINUS:
	if (!isValue(expr[0])) 
	{
	    handleError(expr, "Invalid operation for type");
	    return false;
	}
	type = type_t::INT;
	break;

    case RATE:
	if (!isCost(expr[0]))
	{
	    handleError(expr, "Can only apply rate to cost variables");
	    return false;
	}
	type = type_t::RATE;
	break;

    case ASSIGN:
	if (!areAssignmentCompatible(expr[0].getType(), expr[1].getType())) 
	{
	    handleError(expr, "Incompatible types");
	    return false;
	}
	else if (!isLHSValue(expr[0])) 
	{
	    handleError(expr[0], "Left hand side value expected");
	    return false;
	}
	type = expr[0].getType();
	break;
      
    case ASSPLUS:
	if (!isInteger(expr[0]) && !isCost(expr[0]) || !isInteger(expr[1])) {
	    handleError(expr, "Increment operator can only be used for integer and cost variables.");
	} else if (!isLHSValue(expr[0])) {
	    handleError(expr[0], "Left hand side value expected");
	}
	type = expr[0].getType();
	break;

    case ASSMINUS:
    case ASSDIV:
    case ASSMOD:
    case ASSMULT:
    case ASSAND:
    case ASSOR:
    case ASSXOR:
    case ASSLSHIFT:
    case ASSRSHIFT:
	if (!isValue(expr[0]) || !isValue(expr[1])) 
	{
	    handleError(expr, "Non-integer types must use regular assignment operator");
	    return false;
	}
	else if (!isLHSValue(expr[0])) 
	{
	    handleError(expr[0], "Left hand side value expected");	    
	    return false;
	}
	type = expr[0].getType();
	break;
      
    case POSTINCREMENT:
    case PREINCREMENT:
    case POSTDECREMENT:
    case PREDECREMENT:
	if (!isLHSValue(expr[0])) 
	{
	    handleError(expr[0], "Left hand side value expected");
	    return false;
	}
	else if (!isInteger(expr[0]))
	{
	    handleError(expr, "Integer expected");
	    return false;
	}
	type = type_t::INT;
	break;
    
    case INLINEIF:
	if (!isValue(expr[0])) 
	{
	    handleError(expr, "First argument of inline if must be an integer");
	    return false;
	}
	if (!areInlineIfCompatible(expr[1].getType(), expr[2].getType())) 
	{
	    handleError(expr, "Incompatible arguments to inline if");
	    return false;
	}
	type = expr[1].getType();
	break;
      
    case COMMA:
	if (!isValue(expr[0]) && !isScalar(expr[0]) && !isClock(expr[0]) && !isRecord(expr[0]) && !isVoid(expr[0]) && !isCost(expr[0])) {
	    handleError(expr[0], "Incompatible type for comma expression");
	    return false;
	}
	if (!isValue(expr[1]) && !isScalar(expr[1]) && !isClock(expr[1]) && !isRecord(expr[1]) && !isVoid(expr[1]) && !isCost(expr[1]))
	{
	    handleError(expr[1], "Incompatible type for comma expression");
	    return false;
	}
	type = expr[1].getType();
	break;
      
    case FUNCALL:
	annotate(expr[0]);
	if (expr[0].getType().getBase() != type_t::FUNCTION)
	{
	    handleError(expr[0], "Function name expected");
	    return false;
	}
	/* FIXME: This might produce errors! */
	checkFunctionCallArguments(expr);
	return true;

    case ARRAY:
	arg1 = expr[0].getType();
	arg2 = expr[1].getType();

	/* The left side must be an array.
	 */
	if (arg1.getBase() != type_t::ARRAY)
	{
	    handleError(expr[0], "Array expected");
	    return false;
	}
	type = arg1.getSub();
	
	/* The index must be a value.
	 */
	if (arg1.getArraySize().isInteger() && arg2.isValue())
	{
	    try 
	    {
		Interpreter interpreter(system->getConstantValuation());
		int32_t index = interpreter.evaluate(expr[1]);
		int32_t lower = interpreter.evaluate(arg1.getArraySize().getRange().first);
		int32_t upper = interpreter.evaluate(arg1.getArraySize().getRange().second);
		
		if (index < lower || index > upper)
		{
		    handleError(expr[1], "Array index out of range");
		    return false;
		}
	    }		
	    catch (InterpreterException) 
	    {
		/* We need to rely on run-time type checking instead.
		 */
	    }
	}
	else if (arg1.getArraySize().isScalar() && arg2.isScalar())
	{
	    if (arg1.getArraySize() != arg2)
	    {
		handleError(expr[1], "Incompatible type");
		return false;
	    }
	} 
	break;


    case FORALL:
	checkType(expr[0].getSymbol().getType(), false);

	if (isValue(expr[1]))
	{
	    type = type_t::BOOL;
	}
	else if (isInvariant(expr[1]))
	{
	    type = type_t::INVARIANT;
	}
	else if (isGuard(expr[1]))
	{
	    type = type_t::GUARD;
	}
	else if (isConstraint(expr[1]))
	{
	    type = type_t::CONSTRAINT;
	}
	else
	{
	    handleError(expr[1], "Boolean expected");
	}

	if (!isSideEffectFree(expr[1]))
	{
	    handleError(expr[1], "Expression must be side effect free");
	}
	
	break;

    default:
	return true;
    }
    expr.setType(type);
    return true;
}

/** Returns true if the expression is side effect free. An expression
    is side effect free if it does not modify any variables except
    variables local to functions (and thus not part of the variable
    vector).
*/
bool TypeChecker::isSideEffectFree(expression_t expr) const
{
    return !expr.changesVariable(persistentVariables.get());
}

/** Returns true if expression is a left-hand-side value.
    Left-hand-side values are expressions that result in references to
    variables. Note: An inline if over integers is only a LHS value if
    both results have the same declared range.
*/
bool TypeChecker::isLHSValue(expression_t expr) const
{
    type_t t, f;
    switch (expr.getKind()) 
    {
    case IDENTIFIER:
	return !expr.getSymbol().getType().hasPrefix(prefix::CONSTANT);

    case DOT:
    case ARRAY:
	// REVISIT: What if expr[0] is a process?
	return isLHSValue(expr[0]);
	
    case PREINCREMENT:
    case PREDECREMENT:
    case ASSIGN:
    case ASSPLUS:
    case ASSMINUS:
    case ASSDIV:
    case ASSMOD:
    case ASSMULT:
    case ASSAND:
    case ASSOR:
    case ASSXOR:
    case ASSLSHIFT:
    case ASSRSHIFT:
	return isLHSValue(expr[0]);	    // REVISIT: Maybe skip this
	
    case INLINEIF:
	if (!isLHSValue(expr[1]) || !isLHSValue(expr[2]))
	{
	    return false;
	}
	
	// The annotate() method ensures that the value of the two
	// result arguments are compatible; for integers we
	// additionally require them to have the same (syntactic)
	// range declaration for them to be usable as LHS values.

	t = expr[1].getSymbol().getType();
	f = expr[2].getSymbol().getType();

	while (t.getBase() == type_t::ARRAY) t = t.getSub();
	while (f.getBase() == type_t::ARRAY) f = f.getSub();

	if (t.getBase() == type_t::INT)
	{
	    return t.getRange().first.equal(f.getRange().first)
		&& t.getRange().second.equal(f.getRange().second);
	}
	return true;
      
    case COMMA:
	return isLHSValue(expr[1]);

    case FUNCALL:
	// Functions cannot return references (yet!)

    default:
	return false;
    }
}

/** Returns true if expression is a reference to a unique variable.
    Thus is similar to expr being an LHS value, but in addition we
    require that the reference does not depend on any non-computable
    expressions. Thus i[v] is a LHS value, but if v is a non-constant
    variable, then it does not result in a unique reference.
*/
bool TypeChecker::isUniqueReference(expression_t expr) const
{
    switch (expr.getKind()) 
    {
    case IDENTIFIER:
	return !expr.getType().hasPrefix(prefix::CONSTANT);
    case DOT:
	return isUniqueReference(expr[0]);

    case ARRAY:
	return isUniqueReference(expr[0])
	    && !expr[1].dependsOn(persistentVariables.get());
	
    case PREINCREMENT:
    case PREDECREMENT:
    case ASSIGN:
    case ASSPLUS:
    case ASSMINUS:
    case ASSDIV:
    case ASSMOD:
    case ASSMULT:
    case ASSAND:
    case ASSOR:
    case ASSXOR:
    case ASSLSHIFT:
    case ASSRSHIFT:
	return isUniqueReference(expr[0]);
	
    case INLINEIF:
	return false;
      
    case COMMA:
	return isUniqueReference(expr[1]);

    case FUNCALL:
	// Functions cannot return references (yet!)

    default:
	return false;
    }
}

void TypeChecker::checkExpression(expression_t expr)
{
    annotate(expr);
}

bool parseXTA(FILE *file, ErrorHandler *error, TimedAutomataSystem *system, bool newxta)
{
    SystemBuilder builder(system);
    parseXTA(file, &builder, error, newxta);
    if (!error->hasErrors())
    {
	TypeChecker checker(system, error);
	system->accept(checker);
    }
    return !error->hasErrors();
}

bool parseXTA(const char *buffer, ErrorHandler *error, TimedAutomataSystem *system, bool newxta)
{
    SystemBuilder builder(system);
    parseXTA(buffer, &builder, error, newxta);
    if (!error->hasErrors())
    {
	TypeChecker checker(system, error);
	system->accept(checker);
    }
    return !error->hasErrors();
}

int32_t parseXMLBuffer(const char *buffer, ErrorHandler *error, TimedAutomataSystem *system, bool newxta)
{
    int err;

    SystemBuilder builder(system);
    err = parseXMLBuffer(buffer, &builder, error, newxta);

    if (err)
    {
	return err;
    }

    if (!error->hasErrors())
    {
	TypeChecker checker(system, error);
	system->accept(checker);
    }

    return 0;
}

int32_t parseXMLFile(const char *file, ErrorHandler *error, TimedAutomataSystem *system, bool newxta)
{
    int err;

    SystemBuilder builder(system);
    err = parseXMLFile(file, &builder, error, newxta);
    if (err)
    {
	return err;
    }

    if (!error->hasErrors())
    {
	TypeChecker checker(system, error);
	system->accept(checker);
    }

    return 0;
}

expression_t parseExpression(const char *str, ErrorHandler *error, 
			     TimedAutomataSystem *system, bool newxtr)
{
    ExpressionBuilder builder(system);
    parseXTA(str, &builder, error, newxtr, S_EXPRESSION);
    expression_t expr = builder.getExpressions()[0];
    if (!error->hasErrors())
    {
	TypeChecker checker(system, error);
	checker.checkExpression(expr);
    }
    return expr;
}
