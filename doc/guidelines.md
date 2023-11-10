Code Guidelines and Style

* These are guidelines that should be followed but rigid conformance is not neccassary. If there is
a good reason to not follow them then do what needs to be done. Leaving a comment about why would
not hurt.

* The project started with out clear guidelines for the style to use and the informal style mutated
several times. If you encounter style violations as you work then feel free to fix them as a part
of your work or submit a bug report.

# General

    class SomeClass;
    using SomeVector = std::vector<Thing>;
    
    static const std::string someString("I'm a string!");
    
    int someNonMemberFunction()
    {
        auto numerator = deduceNumerator();
        auto denominator = configuredDenominator()
        
        return numerator / denominator;
    }
    
    if (someCase())
    {
        doThing();
        doOtherThing();
    }
    else
    {
        doElseThing();
        doOtherElseThing();
    }
    
    try {
        doSomething();
    }
    catch (const SomeException& e)
    {
        LOGGER(debug, "oops");
        throw;
    }

* Maximum line width is around 100 characters. Don't worry about going a little over.

* Macros are ok to create but no macro should propagate out to users of the library so keep them
  out of the header files.

* Braces go on new lines.

* Types have an upper case first character.

* Functions and variables have a lower case first character.

# Functions / Methods

    void someFunc(const int in_value, int& out_result)
    {
        out_result = 1 + in_value;
    }
    
* Input arguments are prefixed with in_ and should probably be const

* Ouput arguments are prefixed with out_

# Classes

    class SomeClass
    {
        int m_privateVar;
        
        protected:
        bool m_someFlag;
    
        SomeClass();
        void setPrivateVar(int privateVar);
        
        public:
        static std::string someMethod();
        virtual ~SomeClass();
        bool operator==(const SomeClass& rhs);
        int privateVar();
        bool someFlag();
    }

* Member variables of a class are prefixed with m_.

    * Exception: A struct with only public member variables and static methods doesn't need
      the m_prefix.

* Private members come first, followed by protected, followed by public.

* The private/protected/public designator is indented to the same level as the definitions.

* Variables come before methods and are seperated from methods with a blank line.

* Static methods come first..

* Constructors follow the static methods.

* Destructors follow the constructors.

* Operator overloads follow the destructors.

* All other methods go below operator overloads.

* Don't expose member variables outside of the class. It is ok for a subclass to access a member
  variable if it makes sense.
