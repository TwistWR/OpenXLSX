//
// Created by KBA012 on 16-12-2017.
//

#include "XLCellValue.h"
#include "../XLWorkbook/XLException.h"
#include "../XLSheet/XLWorksheet.h"

using namespace OpenXLSX;
using namespace std;


/**
 * @details The constructor sets the m_value to nullptr and the m_parentCell to the value of the input parameter.
 * Subsequently, the Initialize member function is called, which sets up the m_value variable.
 * @pre The parent input parameter is a valid XLCell object.
 * @post A valid XLCellValue object has been constructed.
 */
XLCellValue::XLCellValue(XLCell &parent)
    : m_parentCell(parent),
      m_valueVariant()
{
    Initialize();
}

/**
 * @details Initializes the object, by creating the right value object, based on the cell type, which is ascertained
 * based on the content of the underlying XML file.
 * @pre The parent XLCell object is valid and has a corresponding node in the underlying XML file.
 * @post The object has been initialized with the correct value type.
 */
void XLCellValue::Initialize()
{
    switch (CellType()) {
        case XLCellType::Empty:
            m_valueVariant = XLValueEmpty(*this);
            break;

        case XLCellType::Number:
            m_valueVariant = XLValueNumber(*this);
            break;

        case XLCellType::String:
            m_valueVariant = XLValueString(*this);
            break;

        case XLCellType::Boolean:
            m_valueVariant = XLValueBoolean(*this);
            break;

        case XLCellType::Error:
            m_valueVariant = XLValueError(*this);
            break;
    }
}

/**
 * @details The copy assignment operator sets the value node and type attribute to the corresponding items in
 * the object being assigned from. Subsequently, the Initialize method is called, which initializes the object
 * based on the underlying XML file.
 * @pre
 * @post
 */
XLCellValue &XLCellValue::operator=(const XLCellValue &other)
{
    SetValueNode(!other.ValueNode() ? "" : other.ValueNode().value());
    SetTypeAttribute(other.TypeString());

    Initialize();
    ParentCell()->SetModified();
    return *this;
}

/**
 * @details The copy assignment operator sets the value node and type attribute to the corresponding items in
 * the object being assigned from. Subsequently, the Initialize method is called, which initializes the object
 * based on the underlying XML file. This is identical to the copy assignment operator.
 * @pre
 * @post
 * @todo Implement a real move assignment operator
 */
XLCellValue &XLCellValue::operator=(XLCellValue &&other) noexcept
{
    SetValueNode(!other.ValueNode() ? "" : other.ValueNode().value());
    SetTypeAttribute(other.TypeString());

    Initialize();
    ParentCell()->SetModified();
    return *this;
}

/**
 * @details The assignment operator taking a std::tring object as a parameter, calls the corresponding Set method
 * and returns the resulting object.
 * @pre
 * @post
 */
XLCellValue &XLCellValue::operator=(const std::string &stringValue)
{
    Set(stringValue);
    return *this;
}

/**
 * @details The assignment operator taking a char* string as a parameter, calls the corresponding Set method
 * and returns the resulting object.
 * @pre
 * @post
 */
XLCellValue &XLCellValue::operator=(const char *stringValue)
{
    Set(stringValue);
    return *this;
}

/**
 * @details If the value type is already a String, the value will be set to the new value. Otherwise, the m_value
 * member variable will be set to an XLString object with the given value.
 * @pre
 * @post
 */
void XLCellValue::Set(string_view stringValue)
{
    if (ValueType() == XLValueType::String)
        std::get<XLValueString>(m_valueVariant).Set(stringValue);
    else
        m_valueVariant = XLValueString(stringValue, *this);

    ParentCell()->SetModified();
}

/**
 * @details Converts the char* parameter to a std::string and calls the corresponding Set method.
 * @pre
 * @post
 */
void XLCellValue::Set(const char *stringValue)
{
    Set(std::string_view(stringValue));
}

/**
 * @details Sets the cell value and type to empty and deletes the value node and type attribute if they exists.
 * @pre The parent XLCell object is valid and has a corresponding node in the underlying XML file.
 * @post The m_value holds an XLEmpty object and the value node and type attributes have been deleted.
 */
void XLCellValue::Clear()
{
    m_valueVariant = XLValueEmpty(*this);
    DeleteValueNode();
    DeleteTypeAttribute();
    ParentCell()->SetModified();
}

/**
 * @details Return the cell value as a string, by calling the AsString method of the m_value member variable.
 * @pre The m_value member variable is valid.
 * @post The current object, and any associated objects, are unchanged.
 */
std::string XLCellValue::AsString() const
{
    switch (m_valueVariant.index()) {
        case 1:
            return std::get<1>(m_valueVariant).AsString();
        case 2:
            return std::get<2>(m_valueVariant).AsString();
        case 3:
            return std::get<3>(m_valueVariant).AsString();
        case 4:
            return std::get<4>(m_valueVariant).AsString();
        case 5:
            return std::get<5>(m_valueVariant).AsString();
    }
}

/**
 * @details Determine the value type, based on the cell type, and return the corresponding XLValueType object.
 * @pre The parent XLCell object is valid and has a corresponding node in the underlying XML file.
 * @post The current object, and any associated objects, are unchanged.
 */
XLValueType XLCellValue::ValueType() const
{
    switch (CellType()) {
        case XLCellType::Empty:
            return XLValueType::Empty;

        case XLCellType::Error:
            return XLValueType::Error;

        case XLCellType::Number: {
            if (std::get<XLValueNumber>(m_valueVariant).NumberType() == XLNumberType::Integer)
                return XLValueType::Integer;
            else
                return XLValueType::Float;
        }

        case XLCellType::Boolean:
            return XLValueType::Boolean;

        default:
            return XLValueType::String;
    }
}

/**
 * @details Determine the cell type, based on the contents of the underlying XML file, and returns the corresponding
 * XLCellType object.
 * @pre The parent XLCell object is valid and has a corresponding node in the underlying XML file.
 * @post The current object, and any associated objects, are unchanged.
 */
XLCellType XLCellValue::CellType() const
{
    // Determine the type of the Cell
    // If neither a Type attribute or a value node is present, the cell is empty.
    if (!m_parentCell.HasTypeAttribute() && !HasValueNode()) {
        return XLCellType::Empty;
    }

        // If a Type attribute is not present, but a value node is, the cell contains a number.
    else if (!m_parentCell.HasTypeAttribute() && HasValueNode()) {
        return XLCellType::Number;
    }

        // If the cell is of type "s", the cell contains a shared string.
    else if (m_parentCell.HasTypeAttribute() && string(m_parentCell.TypeAttribute().value()) == "s") {
        return XLCellType::String;
    }

        // If the cell is of type "inlineStr", the cell contains an inline string.
    else if (m_parentCell.HasTypeAttribute() && string(m_parentCell.TypeAttribute().value()) == "inlineStr") {
        return XLCellType::String;
    }

        // If the cell is of type "str", the cell contains an ordinary string.
    else if (m_parentCell.HasTypeAttribute() && string(m_parentCell.TypeAttribute().value()) == "str") {
        return XLCellType::String;
    }

        // If the cell is of type "b", the cell contains a boolean.
    else if (m_parentCell.HasTypeAttribute() && string(m_parentCell.TypeAttribute().value()) == "b") {
        return XLCellType::Boolean;
    }

        // Otherwise, the cell contains an error.
    else
        return XLCellType::Error; //the m_typeAttribute has the ValueAsString "e"
}

/**
 * @details Returns the type string by calling the TypeString method of the m_value member variable.
 * @pre The m_value member variable is valid.
 * @post The current object, and any associated objects, are unchanged.
 */
std::string XLCellValue::TypeString() const
{
    switch (m_valueVariant.index()) {
        case 1:
            return std::get<1>(m_valueVariant).TypeString();
        case 2:
            return std::get<2>(m_valueVariant).TypeString();
        case 3:
            return std::get<3>(m_valueVariant).TypeString();
        case 4:
            return std::get<4>(m_valueVariant).TypeString();
        case 5:
            return std::get<5>(m_valueVariant).TypeString();
    }
}

/**
 * @details Returns the value of the m_parentCell member variable.
 * @pre The parent XLCell object is valid.
 * @post The current object, and any associated objects, are unchanged.
 */
XLCell *XLCellValue::ParentCell()
{
    return &m_parentCell;
}

/**
 * @details Returns the value of the m_parentCell member variable.
 * @pre The parent XLCell object is valid.
 * @post The current object, and any associated objects, are unchanged.
 */
const XLCell *XLCellValue::ParentCell() const
{
    return &m_parentCell;
}

/**
 * @details If the input value is an empty string, object will be set to empty. Otherwise, a value node will be
 * created (if it doesn't exists) and the value set to the input value.
 * @pre The parent XLCell object is valid and has a corresponding node in the underlying XML file.
 * @post The cell is either set to empty, or a value node exists and has been set to the input value.
 * @note If the input string is empty, the cell type will be set to empty.
 */
void XLCellValue::SetValueNode(string_view value)
{
    if (value.empty()) {
        Clear();
    }
    else {
        CreateValueNode();
        ValueNode().text().set(string(value).c_str());
    }
}

/**
 * @details If a value node exists, it will be deleted.
 * @pre The parent XLCell object is valid and has a corresponding node in the underlying XML file.
 * @post No value node exists for the cell, in the underlying XML file.
 */
void XLCellValue::DeleteValueNode()
{
    if (HasValueNode()) ValueNode().parent().remove_child(ValueNode());
}

/**
 * @details If a value node doesn't exist, one will be created and returned. Otherwise, the existing one will be
 * returned.
 * @pre The parent XLCell object is valid and has a corresponding node in the underlying XML file.
 * @post A value node exists for the cell in the underlying XML file.
 */
XMLNode XLCellValue::CreateValueNode()
{
    if (!HasValueNode()) ParentCell()->CreateValueNode();
    return ValueNode();
}

/**
 * @details If the type string is empty, the type attribute will be deleted. Otherwise, the type attribute will be
 * created and set to the value of the type string.
 * @pre The parent XLCell object is valid and has a corresponding node in the underlying XML file.
 * @post The type attribute is either deleted or has been set to the value of the input type string.
 */
void XLCellValue::SetTypeAttribute(const std::string &typeString)
{
    if (typeString.empty())
        DeleteTypeAttribute();
    else {
        CreateTypeAttribute();
        ParentCell()->CellNode().attribute("t").set_value(typeString.c_str());
    }
}

/**
 * @details If a type attribute exists, it will be deleted from the XML file.
 * @pre The parent XLCell object is valid and has a corresponding node in the underlying XML file.
 * @post No type attribute for the cell exists in the XML file.
 */
void XLCellValue::DeleteTypeAttribute()
{
    if (HasTypeAttribute()) ParentCell()->CellNode().remove_attribute("t");
}

/**
 * @details If a type attribute does not exist, a new one will be created, added to the XML document and returned.
 * Otherwise a pointer to the existing type attribute will be returned.
 * @pre The parent XLCell object is valid and has a corresponding node in the underlying XML file.
 * @post An empty type attribute has been added to the XML file.
 * @note An empty type attribute may not be valid in Excel. Therefore, the value must be set immediately afterwards.
 */
XMLAttribute XLCellValue::CreateTypeAttribute()
{
    if (!HasTypeAttribute())
        ParentCell()->CellNode().append_attribute("t") = "";

    return ParentCell()->CellNode().attribute("t");
}

/**
 * @details Obtain a pointer to the corresponding XMLNode object, requesting child node "v" from the cell node. If
 * no type node exists, a nullptr will be returned.
 * @pre The parent XLCell object is valid and has a corresponding node in the underlying XML file.
 * @post The current object, and any associated objects, are unchanged.
 */
XMLNode XLCellValue::ValueNode()
{
    return ParentCell()->CellNode().child("v");
}

/**
 * @details Obtain a pointer to the corresponding XMLNode object, requesting child node "v" from the cell node. If
 * no type node exists, a nullptr will be returned.
 * @pre The parent XLCell object is valid and has a corresponding node in the underlying XML file.
 * @post The current object, and any associated objects, are unchanged.
 */
const XMLNode XLCellValue::ValueNode() const
{
    return ParentCell()->CellNode().child("v");
}

/**
 * @details Determines the existance of the value node, by checking if a nullptr is returned.
 * @pre The parent XLCell object is valid and has a corresponding node in the underlying XML file.
 * @post The current object, and any associated objects, are unchanged.
 */
bool XLCellValue::HasValueNode() const
{
    if (ParentCell()->CellNode().child("v"))
        return true;
    else
        return false;
}

/**
 * @details Obtain a pointer to the corresponding XMLAttribute object, requesting attribute "t" from the cell node. If
 * no type attribute exists, a nullptr will be returned.
 * @pre The parent XLCell object is valid and has a corresponding node in the underlying XML file.
 * @post The current object, and any associated objects, are unchanged.
 */
XMLAttribute XLCellValue::TypeAttribute()
{
    return ParentCell()->CellNode().attribute("t");
}

/**
 * @details Obtain a pointer to the corresponding XMLAttribute object, requesting attribute "t" from the cell node. If
 * no type attribute exists, a nullptr will be returned.
 * @pre The parent XLCell object is valid and has a corresponding node in the underlying XML file.
 * @post The current object, and any associated objects, are unchanged.
 */
const XMLAttribute XLCellValue::TypeAttribute() const
{
    return ParentCell()->CellNode().attribute("t");
}

/**
 * @details Determines the existance of the type attribute, by checking if a nullptr is returned.
 * @pre The parent XLCell object is valid and has a corresponding node in the underlying XML file.
 * @post The current object, and any associated objects, are unchanged.
 */
bool XLCellValue::HasTypeAttribute() const
{
    if (!ParentCell()->CellNode().attribute("t"))
        return false;
    else
        return true;
}
