Here's how the node docs work.

1:1 relationship from `lib/<module>.js` to `doc/api/<module>.markdown`

Each type of heading has a description block.


    ## module

        Volatility: 2 - Stable

    description and examples.

    ### module.property

    * Type

    description of the property.

    ### module.someFunction(x, y, [z=100])

    * `x` String. the description of the string
    * `y` Boolean. Should I stay or should I go?
    * `z` Number, Optional.  How many zebras to bring.  Default `100`.

    A description of the function.

    ### Event: 'blerg'

    * Argument: SomeClass object.

    Modules don't usually raise events on themselves.  `cluster` is the
    only exception.

    ## Class: SomeClass

    description of the class.

    ### Class Method: SomeClass.classMethod(anArg)

    * `anArg` Object.  Just an argument
      * `field` String. anArg can have this field.
      * `field2` Boolean, Optional.  Another field.  Default: `false`.
    * Return: Boolean.  `true` if it worked.

    Description of the method for humans.

    ### someClass.nextSibling()

    * Return: SomeClass object or null.  The next someClass in line.

    ### someClass.someProperty

    * String

    The indication of what someProperty is.

    ### Event: 'grelb'

    * Argument: Boolean

    This event is emitted on instances of SomeClass, not on the module itself.









* Modules have (description, Properties, Functions, Classes, Examples)
* Properties have (type, description)
* Functions have (list of arguments, description)
* Classes have (description, Properties, Methods, Events)
* Events have (list of arguments, description)
* Methods have (list of arguments, description)
* Properties have (type, description)

## Volatility ratings: 0-5

These can show up below any section heading, and apply to that section.

0 - Locked.  Unless serious bugs are found, this code will not ever
change.  Please do not bother to suggest changes in this area, they will
be refused.

1 - API Frozen.  This API has been tested extensively in production and is
unlikely to ever have to change.

2 - Stable.  The API has proven satisfactory, but cleanup in the underlying
code may cause minor changes.  Backwards-compatibility is guaranteed.

3 - OK.  The API has not had sufficient real-world testing to be considered
stable.  Backwards-compatibility will be maintained if reasonable.

4 - Experimental.  This feature was introduced recently, and may change
in future versions.  Please try it out and provide feedback.

5 - Broken.  This feature is known to be problematic, and changes are
planned.  Do not rely on it.
