
MSEGComponent::MSEGComponent (MSEG::Data& d) : data (d)
{
    setName ("mseg");
}

void MSEGComponent::resized()
{
    dirty = true;
}

void MSEGComponent::setParams (Parameter::Ptr wave_, Parameter::Ptr sync_, Parameter::Ptr rate_,
                               Parameter::Ptr beat_, Parameter::Ptr depth_, Parameter::Ptr offset_,
                               Parameter::Ptr phase_, Parameter::Ptr enable_)
{
    unwatchParams();

    watchParam (wave   = wave_);
    watchParam (sync   = sync_);
    watchParam (rate   = rate_);
    watchParam (beat   = beat_);
    watchParam (depth  = depth_);
    watchParam (offset = offset_);
    watchParam (phase  = phase_);
    watchParam (enable = enable_);

    startTimerHz (30);
}

void MSEGComponent::paramChanged ()
{
    MultiParamComponent::paramChanged();
    dirty = true;
}

void MSEGComponent::createPath (juce::Rectangle<float> area)
{
    mseg.setSampleRate ((double) area.getWidth());

    MSEG::Parameters p;
    p.frequency = 1.0f;

    if (! editable)
    {
        p.phase     = phase->getProcValue();
        p.offset    = offset->getProcValue();
        p.depth     = depth->getProcValue();
    }
    else
    {
        p.phase     = 0.0f;
        p.offset    = 0.0f;
        p.depth     = 1.0f;
    }

    mseg.setParameters (p);
    mseg.reset();

    curve.clear();
    path.clear();

    for (int x = int (area.getX()); x <= area.getRight(); x++)
    {
        auto v = mseg.process (1);

        if (x == int (area.getX()))
            path.startNewSubPath ({float (area.getX()), valueToY (v)});
        else
            path.lineTo ({float (x), valueToY (v)});

        curve[x - int (area.getX())] = valueToY (v);
    }
}

void MSEGComponent::paint (juce::Graphics& g)
{
    auto rc = getLocalBounds().toFloat().reduced (2);

    if (dirty)
    {
        dirty = false;
        createPath (rc);
    }

    g.setColour (dimIfNeeded (findColour (GinLookAndFeel::whiteColourId).withAlpha (0.1f)));

    juce::RectangleList<float> rects;

    if (editable)
    {
        for (int i = 0; i <= 8; i++)
        {
            rects.add ({rc.getX(), rc.getY() + i * rc.getHeight() / 8, rc.getWidth(), 1});
            rects.add ({rc.getX() + i * rc.getWidth() / 8, rc.getY(), 1, rc.getHeight()});
        }
    }
    else
    {
        rects.add ({rc.getX(), rc.getCentreY(), rc.getWidth(), 1});
    }

    g.fillRectList (rects);

    auto c = findColour (GinLookAndFeel::accentColourId).withAlpha (0.7f);

    g.setColour (dimIfNeeded (c));
    g.strokePath (path, juce::PathStrokeType (1.5f));

    if (isEnabled())
    {
        auto lerp = [] (float t, float a, float b)  { return a + t * (b - a); };

        for (auto curPhase : curPhases)
        {
            float x = curPhase * rc.getWidth();
            float t = x - int (x);
            float y = lerp (t, curve[int(x)], curve[int(x) + 1]);

            g.setColour (dimIfNeeded (findColour (GinLookAndFeel::whiteColourId).withAlpha (0.9f)));
            g.fillEllipse (rc.getX() + x - 2, y - 2, 4, 4);
        }
    }

    if (editable && isMouseOverOrDragging())
    {
        for (auto i = 0; i < data.numPoints; i++)
        {
            auto r = juce::Rectangle (timeToX (data.points[i].time) - 2.0f, valueToY (data.points[i].value) - 2.0f, 4.0f, 4.0f);

            if (draggingPoint == i || getPointAt (getMouseXYRelative().toFloat()) == i)
            {
                g.setColour (dimIfNeeded (juce::Colours::white).withAlpha (0.3f));
                g.fillEllipse (r.expanded (4));
            }

            g.setColour (dimIfNeeded (findColour (GinLookAndFeel::accentColourId).withAlpha (0.7f)));
            g.fillEllipse (r);
        }

        for (auto i = 0; i < data.numPoints - 1; i++)
        {
            auto t = (data.points[i].time + data.points[i + 1].time) / 2.0f;
            auto r = juce::Rectangle (timeToX (t) - 2.0f, valueToY (mseg.getValueAt (t)) - 2.0f, 4.0f, 4.0f);

            if (draggingCurve == i || getCurveAt (getMouseXYRelative().toFloat()) == i)
            {
                g.setColour (dimIfNeeded (juce::Colours::white).withAlpha (0.3f));
                g.fillEllipse (r.expanded (4));
            }

            g.setColour (dimIfNeeded (findColour (GinLookAndFeel::accentColourId).withAlpha (0.7f)));
            g.drawEllipse (r, 1.0f);
        }
    }
}

void MSEGComponent::timerCallback()
{
    if (isEnabled() && phaseCallback)
    {
        auto newPhases = phaseCallback();
        if (newPhases != curPhases)
        {
            curPhases = newPhases;
            repaint();
        }
    }
}

float MSEGComponent::valueToY (float v)
{
    auto area = getArea();
    return area.getBottom() - (v + 1.0f) / 2.0f * area.getHeight();
}

float MSEGComponent::timeToX (float t)
{
    auto area = getArea();
    return area.getX() + area.getWidth() * t;
}

float MSEGComponent::xToTime (float x)
{
    auto area = getArea();
    return (x - area.getX()) / area.getWidth();
}

float MSEGComponent::yToValue (float y)
{
    auto area = getArea();
    return 1.0f -((y - area.getY()) / getHeight()) * 2.0f;
}

int MSEGComponent::getPointAt (juce::Point<float> p)
{
    for (auto i = 0; i < data.numPoints; i++)
    {
        auto x = timeToX (data.points[i].time);
        auto y = valueToY (data.points[i].value);

        if (juce::Rectangle<float> (p.x - 3.0f, p.y - 3.0f, 6.0f, 6.0f).contains (x, y))
            return i;
    }
    return -1;
}

int MSEGComponent::getCurveAt (juce::Point<float> p)
{
    for (auto i = 0; i < data.numPoints - 1; i++)
    {
        auto t = (data.points[i].time + data.points[i + 1].time) / 2.0f;

        auto x = timeToX (t);
        auto y = valueToY (mseg.getValueAt (t));

        if (juce::Rectangle<float> (p.x - 3.0f, p.y - 3.0f, 6.0f, 6.0f).contains (x, y))
            return i;
    }
    return -1;
}

void MSEGComponent::mouseDown (const juce::MouseEvent& e)
{
    if (! editable)
        return;

    if ((draggingPoint = getPointAt (e.position)) >= 0)
        repaint ();
    else if ((draggingCurve = getCurveAt (e.position)) >= 0)
        repaint ();

    if (e.getNumberOfClicks() == 2)
    {
        if (draggingPoint >= 0)
        {
            // delete point
            if (draggingPoint == 0 || draggingPoint == data.numPoints - 1)
                return;

            for (int i = draggingPoint; i < data.numPoints - 1; i++)
                data.points.getReference (i) = data.points[i + 1];

            data.numPoints--;

            dirty = true;
            repaint();
        }
        else
        {
            // add point
            if (data.numPoints == data.maxPoints)
                return;

            auto t = xToTime (e.position.x);
            auto v = yToValue (e.position.y);

            for (auto i = 0; i < data.numPoints - 1; i++)
            {
                auto p1 = data.points[i + 0];
                auto p2 = data.points[i + 1];

                if (t > p1.time && t <= p2.time)
                {
                    for (auto j = data.numPoints; j >= i + 1; j--)
                        data.points.getReference (j) = data.points[j - 1];

                    data.points.getReference (i + 1) = { t, v, 0.0f };
                    data.numPoints++;

                    break;
                }
            }

            dirty = true;
            repaint();
        }
    }

    lastY = e.position.y;
}

void MSEGComponent::mouseMove (const juce::MouseEvent&)
{
    if (! editable)
        return;

    repaint();
}

void MSEGComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (! editable)
        return;

    if (draggingPoint >= 0)
    {
        auto& p = data.points.getReference (draggingPoint);

        auto minT = 0.0f;
        auto maxT = 0.0f;

        if (draggingPoint > 0)
            minT = data.points[draggingPoint - 1].time;

        if (draggingPoint < data.numPoints - 1)
            maxT = data.points[draggingPoint + 1].time;

        p.time  = std::clamp (xToTime (e.position.x), minT, maxT);
        p.value = std::clamp (yToValue (e.position.y), -1.0f, 1.0f);

        if (draggingPoint == 0)                  
        {
            p.time = 0.0f;
            data.points.getReference (data.numPoints - 1).value = p.value;
        }
        if (draggingPoint == data.numPoints - 1)
        {
            p.time = 1.0f;
            data.points.getReference (0).value = p.value;
        }

        dirty = true;
        repaint ();
    }
    else if (draggingCurve >= 0)
    {
        auto& p = data.points.getReference (draggingCurve);

        auto factor = easeQuadraticOut (juce::jmap (std::abs (p.curve), 0.0f, 50.0f, 0.0f, 1.0f));
        auto speed  = juce::jmap (factor, 0.0f, 1.0f, 0.01f, 0.2f);
        auto dir    = data.points[draggingCurve].value > data.points[draggingCurve + 1].value ? -1.0f : 1.0f;

        p.curve = std::clamp (p.curve + ((e.y - lastY) * speed * dir), -50.0f, 50.0f);

        dirty = true;
        repaint ();
    }
    lastY = e.position.y;
}

void MSEGComponent::mouseUp (const juce::MouseEvent& e)
{
    if (onClick && e.mouseWasClicked())
        onClick();

    if (! editable)
        return;

    draggingPoint = -1;
    draggingCurve = -1;
    repaint ();
}

void MSEGComponent::mouseEnter (const juce::MouseEvent&)
{
    if (! editable)
        return;

    repaint ();
}

void MSEGComponent::mouseExit (const juce::MouseEvent&)
{
    if (! editable)
        return;

    repaint ();
}
