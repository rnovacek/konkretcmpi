
set(konkretmof_SRCS
    MOF_Buffer.cpp
    MOF_Class_Decl.cpp
    MOF_Data_Type.cpp
    MOF_Date_Time.cpp
    MOF_Element.cpp
    MOF_Error.cpp
    MOF_Feature.cpp
    MOF_Feature_Info.cpp
    MOF_Flavor.cpp
    MOF_Indent.cpp
    MOF_Instance_Decl.cpp
    MOF_Key_Value_Pair.cpp
    MOF_Lex.cpp
    MOF_Lex_Utils.cpp
    MOF_Literal.cpp
    MOF_Method_Decl.cpp
    MOF_Named_Element.cpp
    MOF_Object_Reference.cpp
    MOF_Options.cpp
    MOF_Parameter.cpp
    MOF_Parser.cpp
    MOF_Pragma.cpp
    MOF_Property.cpp
    MOF_Property_Decl.cpp
    MOF_Qualified_Element.cpp
    MOF_Qualifier.cpp
    MOF_Qualifier_Decl.cpp
    MOF_Qualifier_Info.cpp
    MOF_Reference_Decl.cpp
    MOF_String.cpp
    MOF_Yacc.cpp
    REF_Lex.cpp
    REF_Parser.cpp
    REF_Yacc.cpp
)

add_library(konkretmof SHARED ${konkretmof_SRCS})

set_target_properties(konkretmof PROPERTIES VERSION 0.0.1)
set_target_properties(konkretmof PROPERTIES SOVERSION 0)

install(TARGETS konkretmof DESTINATION lib${LIB_SUFFIX})

if (WITH_PYTHON)
    add_subdirectory(python)
endif (WITH_PYTHON)
