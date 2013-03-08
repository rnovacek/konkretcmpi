%module konkretmof

// Suppress the warnings
#pragma SWIG nowarn=314,302,312
%warnfilter(325) MOF_String_Literal;
%warnfilter(325) _ref_init;
%warnfilter(325) _decl_init;
%warnfilter(454) MOF_Qualifier_Decl;
%warnfilter(454) MOF_Class_Decl;
%warnfilter(454) MOF_Instance_Decl;


%{
#include "MOF_Parser.h"
#include "MOF_Types.h"
%}

%include "MOF_Parser.h"
%include "MOF_Types.h"

// Add methods for casting to children
%define Cast(from, to)
%inline %{
to *_Cast_To_##to(from *f) {
    return static_cast<to *>(f);
}
%}
%enddef
Cast(MOF_Element, MOF_Feature_Info)
Cast(MOF_Element, MOF_Key_Value_Pair)
Cast(MOF_Element, MOF_Named_Element)
Cast(MOF_Element, MOF_Qualified_Element)
Cast(MOF_Element, MOF_Class_Decl)
Cast(MOF_Element, MOF_Feature)
Cast(MOF_Element, MOF_Method_Decl)
Cast(MOF_Element, MOF_Property_Decl)
Cast(MOF_Element, MOF_Reference_Decl)
Cast(MOF_Element, MOF_Instance_Decl)
Cast(MOF_Element, MOF_Parameter)
Cast(MOF_Element, MOF_Property)
Cast(MOF_Element, MOF_Qualifier_Decl)
Cast(MOF_Element, MOF_Literal)
Cast(MOF_Element, MOF_Qualifier)
Cast(MOF_Element, MOF_Qualifier_Info)

%pythoncode %{
# Add ability to iterate through objects
class _Iter(object):
    def __init__(self, items, cast):
        self.items = items
        self.cast = cast
    def next(self):
        r = self.items
        if r is None:
            raise StopIteration
        self.items = self.items.next
        return self.cast(r)
MOF_Feature_Info.__iter__ = lambda self: _Iter(self, _Cast_To_MOF_Feature_Info)
MOF_Key_Value_Pair.__iter__ = lambda self: _Iter(self, _Cast_To_MOF_Key_Value_Pair)
MOF_Named_Element.__iter__ = lambda self: _Iter(self, _Cast_To_MOF_Named_Element)
MOF_Qualified_Element.__iter__ = lambda self: _Iter(self, _Cast_To_MOF_Qualified_Element)
MOF_Class_Decl.__iter__ = lambda self: _Iter(self, _Cast_To_MOF_Class_Decl)
MOF_Feature.__iter__ = lambda self: _Iter(self, _Cast_To_MOF_Feature)
MOF_Method_Decl.__iter__ = lambda self: _Iter(self, _Cast_To_MOF_Method_Decl)
MOF_Property_Decl.__iter__ = lambda self: _Iter(self, _Cast_To_MOF_Property_Decl)
MOF_Reference_Decl.__iter__ = lambda self: _Iter(self, _Cast_To_MOF_Reference_Decl)
MOF_Instance_Decl.__iter__ = lambda self: _Iter(self, _Cast_To_MOF_Instance_Decl)
MOF_Parameter.__iter__ = lambda self: _Iter(self, _Cast_To_MOF_Parameter)
MOF_Property.__iter__ = lambda self: _Iter(self, _Cast_To_MOF_Property)
MOF_Qualifier_Decl.__iter__ = lambda self: _Iter(self, _Cast_To_MOF_Qualifier_Decl)
MOF_Literal.__iter__ = lambda self: _Iter(self, _Cast_To_MOF_Literal)
MOF_Qualifier.__iter__ = lambda self: _Iter(self, _Cast_To_MOF_Qualifier)
MOF_Qualifier_Info.__iter__ = lambda self: _Iter(self, _Cast_To_MOF_Qualifier_Info)

def _MOF_Literal_value(self):
    """ Return value of the MOF_Literal. """
    if self.value_type == TOK_INT_VALUE:
        return self.get_int()
    elif self.value_type == TOK_REAL_VALUE:
        return self.get_real()
    elif self.value_type == TOK_CHAR_VALUE:
        return self.get_char()
    elif self.value_type == TOK_BOOL_VALUE:
        return self.get_bool()
    elif self.value_type == TOK_STRING_VALUE:
        return self.get_string()
    else:
        return None
MOF_Literal.value = _MOF_Literal_value

def _properties(self):
    """ Convience method which returns dict of all features of the class
        that are properties or references. Keys are feature names.
    """
    d = {}
    feature = self.features
    while feature:
        f = _Cast_To_MOF_Feature(feature)
        if f.type == MOF_FEATURE_PROP:
            d[f.name] = _Cast_To_MOF_Property_Decl(f)
        elif f.type == MOF_FEATURE_REF:
            d[f.name] = _Cast_To_MOF_Reference_Decl(f)
        feature = feature.next
    return d
MOF_Class_Decl.properties = _properties

def _methods(self):
    """ Convience method which returns dict of all methods of the class.
        Keys are method names.
    """
    d = {}
    feature = self.features
    while feature:
        f = _Cast_To_MOF_Feature(feature)
        if f.type == MOF_FEATURE_METHOD:
            d[f.name] = _Cast_To_MOF_Method_Decl(f)
        feature = feature.next
    return d
MOF_Class_Decl.methods = _methods

def _MOF_Property_Decl_type_name(self):
    return MOF_Data_Type.to_string(self.data_type)
MOF_Property_Decl.type_name = _MOF_Property_Decl_type_name
MOF_Method_Decl.type_name = _MOF_Property_Decl_type_name
MOF_Parameter.type_name = _MOF_Property_Decl_type_name
%}
