// This script attempts to check argument types of printf/wprintf/scanf/wscanf style functions.
// Mostly it's similar to built-in GCC warning functionality, but with the benefit that it can check
// wchar_t* format strings too.
//
// (This is somewhat duplicating the functionality of https://bug493996.bugzilla.mozilla.org/attachment.cgi?id=388700)

include('treehydra.js');
include('gcc_compat.js');
include('gcc_util.js');
include('gcc_print.js');

// Get string corresponding to string literal expressions
function get_string_constant(expr) {
    if (TREE_CODE(expr) == NOP_EXPR)
        return get_string_constant(expr.operands()[0]);
    else if (TREE_CODE(expr) == ADDR_EXPR && TREE_CODE(expr.operands()[0]) == STRING_CST) {
        return expr.operands()[0].string.str;
    }
}

function is_vararg(decl) {
    // Non-vararg functions end with a VOID_TYPE sentinel
    for (var t in flatten_chain(TYPE_ARG_TYPES(TREE_TYPE(decl)))) {
        if (TREE_CODE(TREE_VALUE(t)) == VOID_TYPE)
            return false;
    }
    return true;
}

// Return ['a' (ascii) or 'w' (wide), 'printf' or 'scanf', string-index, first-to-check] or undefined
function find_printf_type(decl, loc) {
    if (! is_vararg(decl))
        return;

    var decl_attrs = translate_attributes(DECL_ATTRIBUTES(decl)); // 'user' attrs are here
    var type_attrs = translate_attributes(TYPE_ATTRIBUTES(TREE_TYPE(decl))); // 'format' attrs are here

    for each (var a in decl_attrs.concat(type_attrs)) {
        if (a.name == 'format') {
            var start = a.value[1];
            var first = a.value[2];
            if (a.value[0] == 'printf')
                return ['a', 'printf', start, first];
            else if (a.value[0] == 'scanf')
                return ['a', 'scanf', start, first];
            else
                error('Unrecognised format attribute type "'+a.value[0]+'"', loc());
        } else if (a.name == 'user' && a.value[0].match(/^format/)) {
            var [ctype, functype, start, first] = a.value[0].split(/,\s*/).slice(1);
            if (first == '+1') first = (+start) + 1; // bit ugly, but lets our macros work easily
            return [ctype, functype, start, first];
        }
    }

    var name = decl_name(decl);

    // Special cases for functions we use and aren't declared with attributes:
    if (name == 'sscanf')
        return ['a', 'scanf', 2, 3];
    else if (name == 'swscanf')
        return ['w', 'scanf', 2, 3];
    else if (name == 'snprintf')
        return ['a', 'printf', 3, 4];
    else if (name == 'wprintf')
        return ['w', 'printf', 1, 2];
    else if (name == 'fwprintf')
        return ['w', 'printf', 2, 3];
    else if (name == 'swprintf')
        return ['w', 'printf', 3, 4];
    else if (name == 'JS_ReportError')
        return ['a', 'printf', 2, 3];
    else if (name == 'JS_ReportWarning')
        return ['a', 'printf', 2, 3];

    // Ignore vararg functions that we know aren't using normal format strings
    if (name.match(/^(__builtin_va_start|execlp|open|fcntl|ioctl|sem_open|h_alloc|sys_wopen|ogl_HaveExtensions|JS_ConvertArguments|curl_easy_setopt|curl_easy_getinfo|SMBIOS::FieldInitializer::Read|SMBIOS::FieldStringizer::Write)$/))
        return;

    warning('Ignoring unannotated vararg function "'+name+'"', loc());
}

function compare_format_type(ctype, functype, fmt, arg, loc) {
    var m, len, spec;
    if (functype == 'printf') {
        m = fmt.match(/^%([-+ #0]*)(\*|\d+)?(\.\*|\.-?\d+)?(hh|h|ll|l|j|z|t|L)?([diouxXfFeEgGaAcspn%])$/);
        if (m) {
            len = m[4] || '';
            spec = m[5];
        }
    } else if (functype == 'scanf') {
        m = fmt.match(/^%(\*?)(\d*)(hh|h|ll|l|j|z|t|L)?([diouxaefgcs[pn%])$/);
        if (m) {
            len = m[3] || '';
            spec = m[4];
        }
    } else {
        error('Internal error: unknown functype '+functype, loc());
        return true;
    }

    if (! spec) {
        error('Invalid format specifier "'+fmt+'"', loc());
        return true;
    }

    var t = len+spec;

    if (ctype == 'w' && t == 's')
        error('Non-portable %s used in wprintf-style function', loc());
    if (ctype == 'a' && t == 'hs')
        error('Illegal %hs used in printf-style function', loc());

    if (functype == 'printf') {
        if (t.match(/^[diouxXc]$/))
            return (arg == 'int' || arg == 'unsigned int');
        if (t.match(/^lc$/))
            return (arg == 'int' || arg == 'unsigned int'); // spec says wint_t
        if (t.match(/^l[diouxX]$/))
            return (arg == 'long int' || arg == 'long unsigned int');
        if (t.match(/^ll[diouxX]$/))
            return (arg == 'long long int' || arg == 'long long unsigned int');
        if (t.match(/^[fFeEgGaA]$/))
            return (arg == 'double');
        if (t.match(/^p$/))
            return (arg.match(/\*$/));
        // ...
    } else if (functype == 'scanf') {
        if (t.match(/^[dioux]$/))
            return (arg == 'int*');
        if (t.match(/^l[diouxX]$/))
            return (arg == 'long int*');
        if (t.match(/^z[diouxX]$/))
            return (arg == 'long unsigned int*'); // spec says size_t*
        if (t.match(/^[c[]$/))
            return (arg == 'char*' || arg == 'unsigned char*');
        if (t.match(/^l[c[]$/))
            return (arg == 'wchar_t*');
        if (t.match(/^[aefg]$/))
            return (arg == 'float*');
        if (t.match(/^l[aefg]$/))
            return (arg == 'double*');
        // ...
    }

    if (t.match(/^h?s$/))
        return (arg.match(/^(const )?(unsigned )?char\*$/));
    if (t.match(/^ls$/))
        return (arg.match(/^(const )?(unsigned )?wchar_t\*$/));

    error('Unrecognized format specifier "'+fmt+'"', loc());
    return true;
}

function check_arg_types(ctype, functype, fmt_string, arg_type_names, loc) {
    // Match a superset of printf and scanf format strings
    var fmt_types = fmt_string.match(/%([-+ #0*]*)(\*|\d+)?(\.\*|\.-?\d+)?(hh|h|ll|l|j|z|t|L)?(.)/g);

    var num_fmt_types = 0;
    for each (var fmt_type in fmt_types) {
        if (fmt_type != '%%')
            ++num_fmt_types;

        // Each '*' eats an extra argument
        var stars = fmt_type.match(/\*/g);
        if (stars)
            num_fmt_types += stars.length;
    }

    if (num_fmt_types != arg_type_names.length) {
        error('Number of format string specifiers ('+num_fmt_types+') != number of format arguments ('+arg_type_names.length+')', loc());
        return;
    }

    for each (var fmt_type in fmt_types) {
        if (fmt_type != '%%') {
            // Each '*' eats an extra argument of type int
            var stars = fmt_type.match(/\*/g);
            if (stars) {
                for (var s in stars) {
                    var arg = arg_type_names.shift();
                    if (! compare_format_type(ctype, functype, '%d', arg, loc)) {
                        error('Invalid argument type "'+arg+'" for format specifier "'+fmt_type+'"', loc());
                    }
                }
            }

            var arg = arg_type_names.shift();
            if (! compare_format_type(ctype, functype, fmt_type, arg, loc)) {
                error('Invalid argument type "'+arg+'" for format specifier "'+fmt_type+'"', loc());
            }
        }
    }
}

function type_string_without_typedefs(type) {
    // Walk up the typedef chain
    while (TYPE_NAME(type) && TREE_CODE(TYPE_NAME(type)) == TYPE_DECL && DECL_ORIGINAL_TYPE(TYPE_NAME(type)))
        type = DECL_ORIGINAL_TYPE(TYPE_NAME(type));
    // Recursively strip typedefs from pointer types
    if (TREE_CODE(type) == POINTER_TYPE) {
        // This bit is copied from type_string():
        let quals = [];
        if (TYPE_VOLATILE(type)) quals.push('volatile');
        if (TYPE_RESTRICT(type)) quals.push('restrict');
        if (TYPE_READONLY(type)) quals.push('const');
        var suffix = quals.length ? ' ' + quals.join(' ') : '';
        return type_string_without_typedefs(TREE_TYPE(type)) + '*' + suffix;
    } else {
        return type_string(type);
    }
}

function walk_printfs(fndecl) {
    function tree_walker(t, stack) {
        function getLocation() {
            var loc = location_of(t);
            if (loc) return loc;
            for (var i = stack.length - 1; i >= 0; --i) {
                var loc = location_of(stack[i]);
                if (loc) return loc;
            }
            return location_of(DECL_SAVED_TREE(fndecl));
        }

        var code = TREE_CODE(t);
        if (code == CALL_EXPR) {
            var decl = call_function_decl(t);
            if (! decl)
                return true;

            var printf_type = find_printf_type(decl, getLocation);
            if (! printf_type)
                return true;

//             print('--------------');
//             print(rectify_function_decl(decl));
//             print(printf_type);

            var fmt_arg = CALL_EXPR_ARG(t, printf_type[2]-1);
            var fmt_string = get_string_constant(fmt_arg);
            if (typeof fmt_string == 'undefined') {
                warning('Non-constant format string argument - can\'t check types', getLocation());
                return true;
            }

            var arg_type_names = [];
            for (var operand in call_arg_iterator(t)) {
                var type = type_string_without_typedefs(TREE_TYPE(operand));
                arg_type_names.push(type);
            }
            check_arg_types(printf_type[0], printf_type[1], fmt_string, arg_type_names.slice(printf_type[3]-1), getLocation);
        }
        return true;
    }
    walk_tree (DECL_SAVED_TREE(fndecl), tree_walker);
}

function process_cp_pre_genericize(fndecl) {
    walk_printfs(fndecl);
}
