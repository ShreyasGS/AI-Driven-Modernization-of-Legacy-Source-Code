/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- 
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is TransforMiiX XSLT processor.
 * 
 * The Initial Developer of the Original Code is The MITRE Corporation.
 * Portions created by MITRE are Copyright (C) 1999 The MITRE Corporation.
 *
 * Portions created by Keith Visco as a Non MITRE employee,
 * (C) 1999 Keith Visco. All Rights Reserved.
 * 
 * Contributor(s): 
 * Keith Visco, kvisco@ziplink.net
 *   -- original author.
 * 
 */

/*
  Implementation of an XPath LocationStep
*/

#include "Expr.h"

/**
 * Creates a new LocationStep using the given NodeExpr and Axis Identifier
 * @param nodeExpr the NodeExpr to use when matching Nodes
 * @param axisIdentifier the Axis Identifier in which to search for nodes
**/
LocationStep::LocationStep(NodeExpr* nodeExpr, short axisIdentifier) : PredicateList() {
    this->nodeExpr = nodeExpr;
    this->axisIdentifier = axisIdentifier;
} //-- LocationStep

/**
 * Destroys this LocationStep
 * All predicates will be deleted
 * The NodeExpr will be deleted
**/
LocationStep::~LocationStep() {
    delete nodeExpr;
} //-- ~LocationStep

  //-----------------------------/
 //- Virtual methods from Expr -/
//-----------------------------/

/**
 * Evaluates this Expr based on the given context node and processor state
 * @param context the context node for evaluation of this Expr
 * @param ps the ProcessorState containing the stack information needed
 * for evaluation
 * @return the result of the evaluation
 * @see Expr
**/
ExprResult* LocationStep::evaluate(Node* context, ContextState* cs) {

    NodeSet* nodes = new NodeSet();
    if (!context || !nodeExpr || !nodes)
        return nodes;

    MBool reverse = MB_FALSE;

    Node* node = context;
    switch (axisIdentifier) {
        case ANCESTOR_AXIS :
            node = context->getXPathParent();
            //-- do not break here
        case ANCESTOR_OR_SELF_AXIS :
            reverse = MB_TRUE;
            while (node) {
                if (nodeExpr->matches(node, context, cs)) {
                    nodes->append(node);
                }
                node = node->getXPathParent();
            }
            break;
        case ATTRIBUTE_AXIS :
        {
            NamedNodeMap* atts = context->getAttributes();
            if (atts) {
                for (PRUint32 i = 0; i < atts->getLength(); i++) {
                    Node* attr = atts->item(i);
                    if (nodeExpr->matches(attr, context, cs))
                        nodes->append(attr);
                }
            }
            break;
        }
        case DESCENDANT_OR_SELF_AXIS :
            if (nodeExpr->matches(context, context, cs))
                nodes->append(context);
            //-- do not break here
        case DESCENDANT_AXIS :
            fromDescendants(context, cs, nodes);
            break;
        case FOLLOWING_AXIS :
        {
            if ( node->getNodeType() == Node::ATTRIBUTE_NODE) {
                node = node->getXPathParent();
                fromDescendants(node, cs, nodes);
            }
            while (node && !node->getNextSibling()) {
                node = node->getXPathParent();
            }
            while (node) {
                node = node->getNextSibling();

                if (nodeExpr->matches(node, context, cs))
                    nodes->append(node);

                if (node->hasChildNodes())
                    fromDescendants(node, cs, nodes);

                while (node && !node->getNextSibling()) {
                    node = node->getParentNode();
                }
            }
            break;
        }
        case FOLLOWING_SIBLING_AXIS :
            node = context->getNextSibling();
            while (node) {
                if (nodeExpr->matches(node, context, cs))
                    nodes->append(node);
                node = node->getNextSibling();
            }
            break;
        case NAMESPACE_AXIS : //-- not yet implemented
#if 0
            // XXX DEBUG OUTPUT
            cout << "namespace axis not yet implemented"<<endl;
#endif
            break;
        case PARENT_AXIS :
        {
            Node* parent = context->getXPathParent();
            if ( nodeExpr->matches(parent, context, cs) )
                    nodes->append(parent);
            break;
        }
        case PRECEDING_AXIS :
            reverse = MB_TRUE;
            while (node && !node->getPreviousSibling()) {
                node = node->getXPathParent();
            }
            while (node) {
                node = node->getPreviousSibling();

                if (node->hasChildNodes())
                    fromDescendantsRev(node, cs, nodes);

                if (nodeExpr->matches(node, context, cs))
                    nodes->append(node);

                while (node && !node->getPreviousSibling()) {
                    node = node->getParentNode();
                }
            }
            break;
        case PRECEDING_SIBLING_AXIS:
            reverse = MB_TRUE;
            node = context->getPreviousSibling();
            while (node) {
                if (nodeExpr->matches(node, context, cs))
                    nodes->append(node);
                node = node->getPreviousSibling();
            }
            break;
        case SELF_AXIS :
            if (nodeExpr->matches(context, context, cs))
                    nodes->append(context);
            break;
        default: //-- Children Axis
        {
            Node* tmpNode = context->getFirstChild();
            while (tmpNode) {
                if (nodeExpr->matches(tmpNode, context, cs))
                    nodes->append(tmpNode);
                tmpNode = tmpNode->getNextSibling();
            }
            break;
        }
    } //-- switch

    //-- apply predicates
    evaluatePredicates(nodes, cs);

    if (reverse)
        nodes->reverse();

    return nodes;
} //-- evaluate

/**
 * Returns the default priority of this Pattern based on the given Node,
 * context Node, and ContextState.
**/
double LocationStep::getDefaultPriority(Node* node, Node* context, ContextState* cs) {
    if (isEmpty())
        return nodeExpr->getDefaultPriority(node, context, cs);
    return 0.5;
} //-- getDefaultPriority


void LocationStep::fromDescendants(Node* context, ContextState* cs, NodeSet* nodes) {

    if ( !context || !nodeExpr ) return;

    Node* child = context->getFirstChild();
    while (child) {
        if (nodeExpr->matches(child, context, cs))
            nodes->append(child);
        //-- check childs descendants
        if (child->hasChildNodes())
            fromDescendants(child, cs, nodes);

        child = child->getNextSibling();
    }

} //-- fromDescendants

void LocationStep::fromDescendantsRev(Node* context, ContextState* cs, NodeSet* nodes) {

    if ( !context || !nodeExpr ) return;

    Node* child = context->getLastChild();
    while (child) {
        //-- check childs descendants
        if (child->hasChildNodes())
            fromDescendantsRev(child, cs, nodes);

        if (nodeExpr->matches(child, context, cs))
            nodes->append(child);

        child = child->getPreviousSibling();
    }

} //-- fromDescendantsRev

/**
 * Determines whether this Expr matches the given node within
 * the given context
**/
MBool LocationStep::matches(Node* node, Node* context, ContextState* cs) {

    if (!nodeExpr || !node)
        return MB_FALSE;

    if (!nodeExpr->matches(node, context, cs))
        return MB_FALSE;

    MBool result = MB_TRUE;
    if (!isEmpty()) {
        NodeSet* nodes = (NodeSet*)evaluate(node->getXPathParent(),cs);
        result = nodes->contains(node);
        delete nodes;
    }
    else if (axisIdentifier == CHILD_AXIS ) {
        if (!node->getParentNode())
            result = MB_FALSE;
    }

    return result;

} //-- matches

/**
 * Creates a String representation of this Expr
 * @param str the destination String to append to
 * @see Expr
**/
void LocationStep::toString(String& str) {
    switch (axisIdentifier) {
        case ANCESTOR_AXIS :
            str.append("ancestor::");
            break;
        case ANCESTOR_OR_SELF_AXIS :
            str.append("ancestor-or-self::");
            break;
        case ATTRIBUTE_AXIS:
            str.append("@");
            break;
        case DESCENDANT_AXIS:
            str.append("descendant::");
            break;
        case DESCENDANT_OR_SELF_AXIS:
            str.append("descendant-or-self::");
            break;
        case FOLLOWING_AXIS :
            str.append("following::");
            break;
        case FOLLOWING_SIBLING_AXIS:
            str.append("following-sibling::");
            break;
        case NAMESPACE_AXIS:
            str.append("namespace::");
            break;
        case PARENT_AXIS :
            str.append("parent::");
            break;
        case PRECEDING_AXIS :
            str.append("preceding::");
            break;
        case PRECEDING_SIBLING_AXIS :
            str.append("preceding-sibling::");
            break;
        case SELF_AXIS :
            str.append("self::");
            break;
        default:
            break;
    }
    if ( nodeExpr ) nodeExpr->toString(str);
    else str.append("null");
    PredicateList::toString(str);
} //-- toString

