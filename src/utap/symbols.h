// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; -*-

/* libutap - Uppaal Timed Automata Parser.
   Copyright (C) 2002 Uppsala University and Aalborg University.
   
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

#ifndef UTAP_SYMBOLS_HH
#define UTAP_SYMBOLS_HH

#include <inttypes.h>
#include <exception>

namespace UTAP
{
    class frame_t;
    class type_t;
    class expression_t;
    
    class NoParentException : public std::exception {};

    /** An integer range. 
     */
    class range_t {
    public:
	int lower, upper;

	/** Constructs the empty range */
	range_t();

	/** Constructs a range containing a single value */
	range_t(int);

	/** Constructs an interval range */
	range_t(int,int);

	/** Constructs an internval range */
	range_t(const std::pair<int,int> &);

	/** Constructs the intersection of two ranges */
	range_t intersect(const range_t &) const;

	/** Constructs the union of two ranges */
	range_t join(const range_t &) const;

	/** Returns true if the argument is contained in the range */
	bool contains(const range_t &) const;

	/** Returns true if the argument is contained in the range */
	bool contains(int32_t) const;

	/** Equallity operator */
	bool operator == (const range_t &) const;

	/** Inequallity operator */
	bool operator != (const range_t &) const;

	/** Constructs the union of two ranges */
	range_t operator| (const range_t &) const;

	/** Constructs the intersection of two ranges */
	range_t operator& (const range_t &) const;

	/** Returns true if and only if the range is empty */
	bool isEmpty() const;

	uint32_t size() const;
    };


    /**
       A reference to a symbol.

       Symbols can only be accessed via instances of
       symbol_t. Internally, symbols are reference counted and do not
       need to be deallocated manually. Each symbol has a name (which
       might be NULL) a type and an uninterpreted optional void
       pointer.

       Symbols are members of a frame (see also frame_t). It is
       possible to access the frame of a symbol via the symbol (see
       getFrame()). However, a symbol does not contain a counted
       reference to its frame so you must maintain at least one
       reference to the frame to avoid to be deallocated.
       
       Notice that it is possible to add the same symbol to several
       frames. In this case, the symbol will only "point back" to the
       first frame it was added to.
    */
    class symbol_t
    {
    private:
	struct symbol_data;
	symbol_data *data;
    protected:
	friend class frame_t;
	symbol_t(void *frame, type_t &type, std::string name, void *user);
    public:
	/** Default constructor */
	symbol_t();

	/** Copy constructor */
	symbol_t(const symbol_t &);
	
	/** Destructor */
	~symbol_t();

	/** Assignment operator */
	const symbol_t &operator = (const symbol_t &);

	/** Equality operator */
	bool operator == (const symbol_t &) const;

	/** Inequality operator */
	bool operator != (const symbol_t &) const;

	/** Less-than operator */
	bool operator < (const symbol_t &) const;
	
	/** Get frame this symbol belongs to */
	frame_t getFrame();

	/** Returns the type of this symbol. */
	type_t getType() const;

	/** Alters the type of this symbol */
	void setType(type_t);
	
	/** Returns the user data of this symbol */
	void *getData();

	/** Return the user data of this symbol */
	const void *getData() const;

	/** Returns the name (identifier) of this symbol */
	std::string getName() const;
	
	/** Sets the user data of this symbol */
	void setData(void *);
    };

    /**
       A reference to a frame.

       A frame is an ordered collection of symbols (see also
       symbol_t). Frames can only be accessed via an instance of
       frame_t. Internally, frames are reference counted and do not
       need to be deallocated manually.

       A frame can either be a root-frame or a sub-frame. Sub-frames
       have a parent frame; root frames do not. When a symbol name
       cannot be resolved in the current frame, it is resolved
       recursively in the parent frame.

       Frames are constructed using one of the static factory methods
       of frame_t. 

       In order to avoid cyclic references no counted reference to the
       parent frame is maintained. Hence, the existence of the parent
       frame must be ensured by other means throughout the lifetime of
       the sub-frame.
    */
    class frame_t
    {
    private:
	struct frame_data;
	frame_data *data;
    protected:
	friend class symbol_t;
	frame_t(void *);
    public:
	/** Default constructor */
	frame_t();

	/** Copy constructor */
	frame_t(const frame_t &);

	/** Destructor */
	~frame_t();

	/** Assignment operator */
	const frame_t &operator = (const frame_t &);

	/** Equality operator */
	bool operator == (const frame_t &) const;

	/** Inequality operator */
	bool operator != (const frame_t &) const;
	
	/** Returns the number of symbols in this frame */
	uint32_t getSize() const;

	/** Returns the Nth symbol in this frame. */
	symbol_t getSymbol(int32_t);

	/** Returns the index of the symbol with the give name. */
	int32_t getIndexOf(std::string name) const;

	/** Returns the Nth symbol in this frame. */
	symbol_t operator[] (int32_t);

	/** Returns the Nth symbol in this frame. */
	const symbol_t operator[] (int32_t) const;

	/** Adds a symbol of the given name and type to the frame */
	symbol_t addSymbol(std::string name, type_t, void *user = NULL);

	/** Add all symbols from the given frame */
	void add(frame_t);
	
	/** Resolves a name in this frame or a parent frame. */
	bool resolve(std::string name, symbol_t &symbol);

	/** Returns the parent frame */
	frame_t getParent() throw (NoParentException);

	/** Returns true if this frame has a parent */
	bool hasParent() const;

	/** Creates and returns a new root-frame. */
	static frame_t createFrame();

	/** Creates and returns a new sub-frame. */
	static frame_t createFrame(const frame_t &parent);
    };

    namespace prefix {
	enum prefix_t {
	    URGENT = 1,
	    COMMITTED = 2,
	    CONSTANT = 4,
	    BROADCAST = 8,
	    REFERENCE = 16,
	    META = 32,
            WINNING = 64,
            LOSING = 128
	};
    }

    /**
       A reference to a type.

       Types are represented as type objects. Type objects cannot be
       access directly. You need to use an instance of type_t to
       access a type object. Internally, type objects are reference
       counted and do not need to be deallocated manually. 

       Types are either primitive such as clocks or channels, or
       contructed types such as structs and array.  Primitive types
       are allocated statically and can be accessed via the static
       member fields of the type_t class. Constructed types are
       created using one of the static factory methods in the type_t
       class.

       All types have a base type. For primitive types, the base type
       is the type itself. For constructed types, the base type
       indicates the type constructor (i.e. if this is an array,
       record, etc.). In addition, all types can have a number of
       prefixes, such as URGENT, COMMITTED, CONSTANT, BROADCAST, and
       REFERENCE.

       Constructed types can have other fields: Integers have a range,
       arrays have a size and a sub-type, records have fields,
       functions have parameters and a return type (the sub-type),
       templates have parameters, and named types have a sub-type.
    */
    class type_t
    {
    private:
	struct type_data;
	type_data *data;
	type_t(void *);
    public:
	/** Default constructor */
	type_t();

	/** Copy constructor */
	type_t(const type_t &);

	/** Destructor */
	~type_t();

	/** Assignment operator */
	const type_t &operator = (const type_t &);

	/** Equality operator */
	bool operator == (const type_t &) const;

	/** Inequality operator */
	bool operator != (const type_t &) const;
	
	/** Returns the base type of this frame. */
	type_t getBase() const;

	/** Returns the fields of a record type */
	frame_t getRecordFields() const;

	/** Returns the parameters of a function or template type */
	frame_t getParameters() const;

	/** Polymorphic version of getRecordFields() and getParameters() */
	frame_t getFrame() const;
	
	/** Returns the sub-type of this type. */
	type_t getSub();

 	/** Returns the return-type of a function. */
 	type_t getReturnType();
 
	/** Returns the true if this type has the given prefix */
	bool hasPrefix(prefix::prefix_t) const;

	/** Sets or clears a prefix. */
	type_t setPrefix(bool set, prefix::prefix_t) const;

	/** Returns the size of an array */
	type_t getArraySize() const;

	/** Returns the range of an integer type. */
	std::pair<expression_t, expression_t> getRange() const;

	/** Print type to the given output stream. */
	std::string toString();

	/** Returns true if this is an integer. */
	bool isInteger() const {
	    return getBase() == type_t::INT;
	}

	/** Returns true if this is a boolean or integer. */
	bool isValue() const {
	    return getBase() == type_t::INT || getBase() == type_t::BOOL;
	}

	/** Returns true if this is a scalar or integer. */
	bool isScalar() const {
	    return getBase() == type_t::SCALAR || isInteger();
	}

	/** Returns true if this is a clock. */
	bool isClock() const {
	    return getBase() == type_t::CLOCK;
	}

	/** Returns true if this is a record. */
	bool isRecord() const {
	    return getBase() == type_t::RECORD;
	}
	
	/** Returns true if this is a clock difference. */
	bool isDiff() const {
	    return getBase() == type_t::DIFF;
	}


	/** Returns true if this is a void. */
	bool isVoid() const {
	    return getBase() == type_t::VOID_TYPE;
	}

	/* Returns true if this is an invariant, boolean or integer. */
	bool isInvariant() const {
	    return getBase() == type_t::INVARIANT || isValue();
	}

	/* Returns true if this is a guard, invariant, boolean or integer. */
	bool isGuard() const {
	    return getBase() == type_t::GUARD || isInvariant();
	}

	/* Returns true if this is a constraint, guard, invariant,
	 * boolean or integer. 
	 */
	bool isConstraint() const {
	    return getBase() == type_t::CONSTRAINT || isGuard();
	}

	bool isArray() const {
	    return getBase() == type_t::ARRAY;
	}

	/** Creates and returns a new integer type with the given range */
	static type_t createInteger(expression_t, expression_t);

	/** Create and returns a new scalar set type of the given size */
	static type_t createScalarSet(expression_t, expression_t);
	
	/** Creates and returns a new record type */
	static type_t createRecord(frame_t);

	/** Creates and returns a new function type */
	static type_t createFunction(frame_t, type_t);

	/** Creates and returns a new array type */
	static type_t createArray(type_t, type_t);

	/** Creates and returns a new named type */
	static type_t createTypeName(type_t);

	/** Creates and returns a new template type */
	static type_t createTemplate(frame_t);

	/** Creates and returns a new process type */
	static type_t createProcess(frame_t);

	/** Create a new primitive type */
	static type_t createBase();

	static type_t UNKNOWN;
	static type_t VOID_TYPE;
	static type_t CLOCK;
	static type_t INT;
	static type_t BOOL;
	static type_t SCALAR;
	static type_t LOCATION;
	static type_t CHANNEL;
	static type_t TEMPLATE;
	static type_t INSTANCE;
	static type_t FUNCTION;
	static type_t ARRAY;
	static type_t RECORD;
	static type_t PROCESS;
	static type_t NTYPE;
	static type_t INVARIANT;
	static type_t INVARIANT_WR;  /* with rate */
	static type_t GUARD;
	static type_t DIFF;
	static type_t CONSTRAINT;
	static type_t COST;
	static type_t RATE;
    };
}

std::ostream &operator << (std::ostream &o, UTAP::type_t t);

#endif
