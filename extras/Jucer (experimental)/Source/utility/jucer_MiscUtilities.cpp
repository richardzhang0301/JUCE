/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-10 by Raw Material Software Ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the GNU General
   Public License (Version 2), as published by the Free Software Foundation.
   A copy of the license is included in the JUCE distribution, or can be found
   online at www.gnu.org/licenses.

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.rawmaterialsoftware.com/juce for more information.

  ==============================================================================
*/

#include "../jucer_Headers.h"
#include "jucer_FillTypePropertyComponent.h"


//==============================================================================
const int64 hashCode64 (const String& s)
{
    return s.hashCode64() + s.length() * s.hashCode() + s.toUpperCase().hashCode();
}

const String createAlphaNumericUID()
{
    String uid;
    static const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    Random r (Random::getSystemRandom().nextInt64());

    for (int i = 7; --i >= 0;)
    {
        r.setSeedRandomly();
        uid << chars [r.nextInt (numElementsInArray (chars))];
    }

    return uid;
}

const String randomHexString (Random& random, int numChars)
{
    String s;
    const char hexChars[] = "0123456789ABCDEF";

    while (--numChars >= 0)
        s << hexChars [random.nextInt (16)];

    return s;
}

const String hexString8Digits (int value)
{
    return String::toHexString (value).paddedLeft ('0', 8);
}

const String createGUID (const String& seed)
{
    String guid;
    Random r (hashCode64 (seed + "_jucersalt"));
    guid << "{" << randomHexString (r, 8); // (written as separate statements to enforce the order of execution)
    guid << "-" << randomHexString (r, 4);
    guid << "-" << randomHexString (r, 4);
    guid << "-" << randomHexString (r, 4);
    guid << "-" << randomHexString (r, 12) << "}";
    return guid;
}

//==============================================================================
void autoScrollForMouseEvent (const MouseEvent& e)
{
    Viewport* const viewport = e.eventComponent->findParentComponentOfClass ((Viewport*) 0);

    if (viewport != 0)
    {
        const MouseEvent e2 (e.getEventRelativeTo (viewport));
        viewport->autoScroll (e2.x, e2.y, 8, 16);
    }
}

void drawComponentPlaceholder (Graphics& g, int w, int h, const String& text)
{
    g.fillAll (Colours::white.withAlpha (0.4f));
    g.setColour (Colours::grey);
    g.drawRect (0, 0, w, h);

    g.drawLine (0.5f, 0.5f, w - 0.5f, h - 0.5f);
    g.drawLine (0.5f, h - 0.5f, w - 0.5f, 0.5f);

    g.setColour (Colours::black);
    g.setFont (11.0f);
    g.drawFittedText (text, 2, 2, w - 4, h - 4, Justification::centredTop, 2);
}

void drawRecessedShadows (Graphics& g, int w, int h, int shadowSize)
{
    ColourGradient cg (Colours::black.withAlpha (0.15f), 0, 0,
                       Colours::transparentBlack, 0, (float) shadowSize, false);
    cg.addColour (0.4, Colours::black.withAlpha (0.07f));
    cg.addColour (0.6, Colours::black.withAlpha (0.02f));

    g.setGradientFill (cg);
    g.fillRect (0, 0, w, shadowSize);

    cg.point1.setXY (0.0f, (float) h);
    cg.point2.setXY (0.0f, (float) h - shadowSize);
    g.setGradientFill (cg);
    g.fillRect (0, h - shadowSize, w, shadowSize);

    cg.point1.setXY (0.0f, 0.0f);
    cg.point2.setXY ((float) shadowSize, 0.0f);
    g.setGradientFill (cg);
    g.fillRect (0, 0, shadowSize, h);

    cg.point1.setXY ((float) w, 0.0f);
    cg.point2.setXY ((float) w - shadowSize, 0.0f);
    g.setGradientFill (cg);
    g.fillRect (w - shadowSize, 0, shadowSize, h);
}

//==============================================================================
int indexOfLineStartingWith (const StringArray& lines, const String& text, int startIndex)
{
    startIndex = jmax (0, startIndex);

    while (startIndex < lines.size())
    {
        if (lines[startIndex].trimStart().startsWithIgnoreCase (text))
            return startIndex;

        ++startIndex;
    }

    return -1;
}


//==============================================================================
PropertyPanelWithTooltips::PropertyPanelWithTooltips()
    : lastComp (0)
{
    addAndMakeVisible (panel = new PropertyPanel());
    startTimer (150);
}

PropertyPanelWithTooltips::~PropertyPanelWithTooltips()
{
    deleteAllChildren();
}

void PropertyPanelWithTooltips::paint (Graphics& g)
{
    g.setColour (Colour::greyLevel (0.15f));
    g.setFont (13.0f);

    TextLayout tl;
    tl.appendText (lastTip, Font (14.0f));
    tl.layout (getWidth() - 10, Justification::left, true); // try to make it look nice
    if (tl.getNumLines() > 3)
        tl.layout (getWidth() - 10, Justification::left, false); // too big, so just squash it in..

    tl.drawWithin (g, 5, panel->getBottom() + 2, getWidth() - 10,
                   getHeight() - panel->getBottom() - 4,
                   Justification::centredLeft);
}

void PropertyPanelWithTooltips::resized()
{
    panel->setBounds (0, 0, getWidth(), jmax (getHeight() - 60, proportionOfHeight (0.6f)));
}

void PropertyPanelWithTooltips::timerCallback()
{
    Component* const newComp = Desktop::getInstance().getMainMouseSource().getComponentUnderMouse();

    if (newComp != lastComp)
    {
        lastComp = newComp;

        String newTip (findTip (newComp));

        if (newTip != lastTip)
        {
            lastTip = newTip;
            repaint (0, panel->getBottom(), getWidth(), getHeight());
        }
    }
}

const String PropertyPanelWithTooltips::findTip (Component* c)
{
    while (c != 0 && c != this)
    {
        TooltipClient* const tc = dynamic_cast <TooltipClient*> (c);
        if (tc != 0)
        {
            const String tip (tc->getTooltip());

            if (tip.isNotEmpty())
                return tip;
        }

        c = c->getParentComponent();
    }

    return String::empty;
}

//==============================================================================
FloatingLabelComponent::FloatingLabelComponent()
    : font (10.0f)
{
    setInterceptsMouseClicks (false ,false);
}

void FloatingLabelComponent::remove()
{
    if (getParentComponent() != 0)
        getParentComponent()->removeChildComponent (this);
}

void FloatingLabelComponent::update (Component* parent, const String& text, const Colour& textColour, int x, int y, bool toRight, bool below)
{
    colour = textColour;

    Rectangle<int> r;

    if (text != getName())
    {
        setName (text);
        glyphs.clear();
        glyphs.addJustifiedText (font, text, 0, 0, 200.0f, Justification::left);
        glyphs.justifyGlyphs (0, std::numeric_limits<int>::max(), 0, 0, 1000, 1000, Justification::topLeft);

        r = glyphs.getBoundingBox (0, std::numeric_limits<int>::max(), false)
                  .getSmallestIntegerContainer().expanded (2, 2);
    }
    else
    {
        r = getLocalBounds();
    }

    r.setPosition (x + (toRight ? 3 : -(r.getWidth() + 3)), y + (below ? 2 : -(r.getHeight() + 2)));
    setBounds (r);
    parent->addAndMakeVisible (this);
}

void FloatingLabelComponent::paint (Graphics& g)
{
    g.setFont (font);
    g.setColour (Colours::white.withAlpha (0.5f));

    for (int y = -1; y <= 1; ++y)
        for (int x = -1; x <= 1; ++x)
            glyphs.draw (g, AffineTransform::translation (1.0f + x, 1.0f + y));

    g.setColour (colour);
    glyphs.draw (g, AffineTransform::translation (1.0f, 1.0f));
}

//==============================================================================
RelativeRectangleLayoutManager::RelativeRectangleLayoutManager (Component* parentComponent)
    : parent (parentComponent)
{
    parent->addComponentListener (this);
}

RelativeRectangleLayoutManager::~RelativeRectangleLayoutManager()
{
    parent->removeComponentListener (this);

    for (int i = components.size(); --i >= 0;)
        components.getUnchecked(i)->component->removeComponentListener (this);
}

void RelativeRectangleLayoutManager::setMarker (const String& name, const RelativeCoordinate& coord)
{
    for (int i = markers.size(); --i >= 0;)
    {
        MarkerPosition* m = markers.getUnchecked(i);
        if (m->markerName == name)
        {
            m->position = coord;
            applyLayout();
            return;
        }
    }

    markers.add (new MarkerPosition (name, coord));
    applyLayout();
}

void RelativeRectangleLayoutManager::setComponentBounds (Component* comp, const String& name, const RelativeRectangle& coords)
{
    jassert (comp != 0);

    // All the components that this layout manages must be inside the parent component..
    jassert (parent->isParentOf (comp));

    for (int i = components.size(); --i >= 0;)
    {
        ComponentPosition* c = components.getUnchecked(i);
        if (c->component == comp)
        {
            c->name = name;
            c->coords = coords;
            triggerAsyncUpdate();
            return;
        }
    }

    components.add (new ComponentPosition (comp, name, coords));
    comp->addComponentListener (this);
    triggerAsyncUpdate();
}

void RelativeRectangleLayoutManager::applyLayout()
{
    for (int i = components.size(); --i >= 0;)
    {
        ComponentPosition* c = components.getUnchecked(i);

        // All the components that this layout manages must be inside the parent component..
        jassert (parent->isParentOf (c->component));

        c->component->setBounds (c->coords.resolve (this).getSmallestIntegerContainer());
    }
}

const RelativeCoordinate RelativeRectangleLayoutManager::findNamedCoordinate (const String& objectName, const String& edge) const
{
    if (objectName == RelativeCoordinate::Strings::parent)
    {
        if (edge == RelativeCoordinate::Strings::right)     return RelativeCoordinate ((double) parent->getWidth(), true);
        if (edge == RelativeCoordinate::Strings::bottom)    return RelativeCoordinate ((double) parent->getHeight(), false);
    }

    if (objectName.isNotEmpty() && edge.isNotEmpty())
    {
        for (int i = components.size(); --i >= 0;)
        {
            ComponentPosition* c = components.getUnchecked(i);

            if (c->name == objectName)
            {
                if (edge == RelativeCoordinate::Strings::left)   return c->coords.left;
                if (edge == RelativeCoordinate::Strings::right)  return c->coords.right;
                if (edge == RelativeCoordinate::Strings::top)    return c->coords.top;
                if (edge == RelativeCoordinate::Strings::bottom) return c->coords.bottom;
            }
        }
    }

    for (int i = markers.size(); --i >= 0;)
    {
        MarkerPosition* m = markers.getUnchecked(i);

        if (m->markerName == objectName)
            return m->position;
    }

    return RelativeCoordinate();
}

void RelativeRectangleLayoutManager::componentMovedOrResized (Component& component, bool wasMoved, bool wasResized)
{
    triggerAsyncUpdate();

    if (parent == &component)
        handleUpdateNowIfNeeded();
}

void RelativeRectangleLayoutManager::componentBeingDeleted (Component& component)
{
    for (int i = components.size(); --i >= 0;)
    {
        ComponentPosition* c = components.getUnchecked(i);
        if (c->component == &component)
        {
            components.remove (i);
            break;
        }
    }
}

void RelativeRectangleLayoutManager::handleAsyncUpdate()
{
    applyLayout();
}

RelativeRectangleLayoutManager::MarkerPosition::MarkerPosition (const String& name, const RelativeCoordinate& coord)
    : markerName (name), position (coord)
{
}

RelativeRectangleLayoutManager::ComponentPosition::ComponentPosition (Component* component_, const String& name_, const RelativeRectangle& coords_)
    : component (component_), name (name_), coords (coords_)
{
}

//==============================================================================
const ColourGradient FillTypeEditorComponent::getDefaultGradient() const
{
    FillTypePropertyComponent* p = dynamic_cast <FillTypePropertyComponent*> (getParentComponent());
    jassert (p != 0);
    return p->getDefaultGradient();
}


//==============================================================================
CallOutBox::CallOutBox (Component& content_, const float arrowSize_)
    : edge (jmax (20, (int) arrowSize_)), content (content_), arrowSize (arrowSize_)
{
    addAndMakeVisible (&content);
}

CallOutBox::~CallOutBox()
{
}

//==============================================================================
void CallOutBox::showModal (Component& content, Component* targetComp, Component* parentComp, float arrowSize)
{
    showModal (content,
               parentComp,
               parentComp == 0 ? targetComp->getParentMonitorArea()
                               : parentComp->getLocalBounds(),
               parentComp == 0 ? targetComp->getScreenBounds()
                               : (targetComp->getLocalBounds() + targetComp->relativePositionToOtherComponent (parentComp, Point<int>())),
               arrowSize);
}

void CallOutBox::showModal (Component& content, Component* parent,
                            const Rectangle<int>& availableAreaInParent,
                            const Rectangle<int>& targetAreaInParent, float arrowSize)
{
    CallOutBox p (content, arrowSize);
    p.updatePosition (targetAreaInParent, availableAreaInParent);

    if (parent != 0)
        parent->addAndMakeVisible (&p);
    else
        p.addToDesktop (ComponentPeer::windowIsTemporary);

    p.runModalLoop();
}

//==============================================================================
void CallOutBox::paint (Graphics& g)
{
    if (background.isNull())
    {
        DropShadowEffect shadow;
        shadow.setShadowProperties (5.0f, 0.4f, 0, 2);

        Image im (Image::ARGB, getWidth(), getHeight(), true);

        {
            Graphics g (im);

            g.setColour (Colour::greyLevel (0.23f).withAlpha (0.9f));
            g.fillPath (outline);

            g.setColour (Colours::white.withAlpha (0.8f));
            g.strokePath (outline, PathStrokeType (2.0f));
        }

        background = Image (Image::ARGB, getWidth(), getHeight(), true);
        Graphics g (background);
        shadow.applyEffect (im, g);
    }

    g.setColour (Colours::black);
    g.drawImageAt (background, 0, 0);
}

void CallOutBox::resized()
{
    content.setTopLeftPosition (edge, edge);
    refreshPath();
}

void CallOutBox::moved()
{
    refreshPath();
}

void CallOutBox::childBoundsChanged (Component*)
{
    updatePosition (targetArea, availableArea);
}

bool CallOutBox::hitTest (int x, int y)
{
    return outline.contains ((float) x, (float) y);
}

void CallOutBox::inputAttemptWhenModal()
{
    exitModalState (0);
    setVisible (false);
}

void CallOutBox::updatePosition (const Rectangle<int>& newTargetArea, const Rectangle<int>& newArea)
{
    targetArea = newTargetArea;
    availableArea = newArea;

    Rectangle<int> bounds (0, 0,
                           content.getWidth() + edge * 2,
                           content.getHeight() + edge * 2);

    const int hw = bounds.getWidth() / 2;
    const int hh = bounds.getHeight() / 2;
    const float hwReduced = (float) (hw - edge * 3);
    const float hhReduced = (float) (hh - edge * 3);
    const float arrowIndent = edge - arrowSize;

    Point<float> targets[4] = { Point<float> ((float) targetArea.getCentreX(), (float) targetArea.getBottom()),
                                Point<float> ((float) targetArea.getRight(), (float) targetArea.getCentreY()),
                                Point<float> ((float) targetArea.getX(), (float) targetArea.getCentreY()),
                                Point<float> ((float) targetArea.getCentreX(), (float) targetArea.getY()) };

    Line<float> lines[4] = { Line<float> (targets[0].translated (-hwReduced, hh - arrowIndent), targets[0].translated (hwReduced, hh - arrowIndent)),
                             Line<float> (targets[1].translated (hw - arrowIndent, -hhReduced), targets[1].translated (hw - arrowIndent, hhReduced)),
                             Line<float> (targets[2].translated (-(hw - arrowIndent), -hhReduced), targets[2].translated (-(hw - arrowIndent), hhReduced)),
                             Line<float> (targets[3].translated (-hwReduced, -(hh - arrowIndent)), targets[3].translated (hwReduced, -(hh - arrowIndent))) };

    const Rectangle<float> reducedArea (newArea.reduced (hw, hh).toFloat());

    float bestDist = 1.0e9f;

    for (int i = 0; i < 4; ++i)
    {
        Line<float> constrainedLine (reducedArea.getConstrainedPoint (lines[i].getStart()),
                                     reducedArea.getConstrainedPoint (lines[i].getEnd()));

        const Point<float> centre (constrainedLine.findNearestPointTo (reducedArea.getCentre()));
        float distanceFromCentre = centre.getDistanceFrom (reducedArea.getCentre());

        if (! (reducedArea.contains (lines[i].getStart()) || reducedArea.contains (lines[i].getEnd())))
            distanceFromCentre *= 2.0f;

        if (distanceFromCentre < bestDist)
        {
            bestDist = distanceFromCentre;

            targetPoint = targets[i];
            bounds.setPosition ((int) (centre.getX() - hw),
                                (int) (centre.getY() - hh));
        }
    }

    setBounds (bounds);
}

void CallOutBox::refreshPath()
{
    repaint();
    background = Image();
    outline.clear();

    const float gap = 4.5f;
    const float cornerSize = 9.0f;
    const float cornerSize2 = 2.0f * cornerSize;
    const float arrowBaseWidth = arrowSize * 0.7f;
    const float left = content.getX() - gap, top = content.getY() - gap, right = content.getRight() + gap, bottom = content.getBottom() + gap;
    const float targetX = targetPoint.getX() - getX(), targetY = targetPoint.getY() - getY();

    outline.startNewSubPath (left + cornerSize, top);

    if (targetY < edge)
    {
        outline.lineTo (targetX - arrowBaseWidth, top);
        outline.lineTo (targetX, targetY);
        outline.lineTo (targetX + arrowBaseWidth, top);
    }

    outline.lineTo (right - cornerSize, top);
    outline.addArc (right - cornerSize2, top, cornerSize2, cornerSize2, 0, float_Pi * 0.5f);

    if (targetX > right)
    {
        outline.lineTo (right, targetY - arrowBaseWidth);
        outline.lineTo (targetX, targetY);
        outline.lineTo (right, targetY + arrowBaseWidth);
    }

    outline.lineTo (right, bottom - cornerSize);
    outline.addArc (right - cornerSize2, bottom - cornerSize2, cornerSize2, cornerSize2, float_Pi * 0.5f, float_Pi);

    if (targetY > bottom)
    {
        outline.lineTo (targetX + arrowBaseWidth, bottom);
        outline.lineTo (targetX, targetY);
        outline.lineTo (targetX - arrowBaseWidth, bottom);
    }

    outline.lineTo (left + cornerSize, bottom);
    outline.addArc (left, bottom - cornerSize2, cornerSize2, cornerSize2, float_Pi, float_Pi * 1.5f);

    if (targetX < left)
    {
        outline.lineTo (left, targetY + arrowBaseWidth);
        outline.lineTo (targetX, targetY);
        outline.lineTo (left, targetY - arrowBaseWidth);
    }

    outline.lineTo (left, top + cornerSize);
    outline.addArc (left, top, cornerSize2, cornerSize2, float_Pi * 1.5f, float_Pi * 2.0f - 0.05f);

    outline.closeSubPath();
}
