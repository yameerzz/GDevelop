/** \file
 *  Game Develop
 *  2008-2014 Florian Rival (Florian.Rival@gmail.com)
 */

#include "WhileEvent.h"
#include <wx/msgdlg.h>
#include "GDCore/IDE/CommonBitmapManager.h"
#include "GDCore/TinyXml/tinyxml.h"
#include "GDCore/Events/EventsCodeGenerator.h"
#include "GDCore/Events/ExpressionsCodeGeneration.h"
#include "GDCore/IDE/EventsRenderingHelper.h"
#include "GDCore/IDE/EventsEditorItemsAreas.h"
#include "GDCore/IDE/EventsEditorSelection.h"
#include "GDCore/Events/Serialization.h"
#include "GDCore/Events/EventsCodeGenerationContext.h"

using namespace std;

namespace gd
{

vector < vector<gd::Instruction>* > WhileEvent::GetAllConditionsVectors()
{
    vector < vector<gd::Instruction>* > allConditions;
    allConditions.push_back(&whileConditions);
    allConditions.push_back(&conditions);

    return allConditions;
}

vector < vector<gd::Instruction>* > WhileEvent::GetAllActionsVectors()
{
    vector < vector<gd::Instruction>* > allActions;
    allActions.push_back(&actions);

    return allActions;
}

void WhileEvent::SaveToXml(TiXmlElement * eventElem) const
{
    if ( eventElem == NULL ) return;

    eventElem->SetAttribute("infiniteLoopWarning", infiniteLoopWarning ? "true" : "false");

    //Save "While conditions"
    TiXmlElement * whileConditionsElem = new TiXmlElement( "WhileConditions" );
    eventElem->LinkEndChild( whileConditionsElem );
    gd::EventsListSerialization::SaveConditions(whileConditions, whileConditionsElem);

    //Les conditions
    TiXmlElement * conditionsElem = new TiXmlElement( "Conditions" );
    eventElem->LinkEndChild( conditionsElem );
    gd::EventsListSerialization::SaveConditions(conditions, conditionsElem);

    //Les actions
    TiXmlElement * actionsElem = new TiXmlElement( "Actions" );
    eventElem->LinkEndChild( actionsElem );
    gd::EventsListSerialization::SaveActions(actions, actionsElem);

    //Sous �v�nements
    if ( !GetSubEvents().empty() )
    {
        TiXmlElement * subeventsElem;
        subeventsElem = new TiXmlElement( "Events" );
        eventElem->LinkEndChild( subeventsElem );

        gd::EventsListSerialization::SaveEventsToXml(events, subeventsElem);
    }
}


void WhileEvent::LoadFromXml(gd::Project & project, const TiXmlElement * eventElem)
{
    if ( eventElem == NULL ) return;

    infiniteLoopWarning = (eventElem->Attribute("infiniteLoopWarning") == NULL || std::string(eventElem->Attribute("infiniteLoopWarning")) == "true");
    justCreatedByTheUser = false;

    if ( eventElem->FirstChildElement( "WhileConditions" ) != NULL )
        gd::EventsListSerialization::OpenConditions(project, whileConditions, eventElem->FirstChildElement( "WhileConditions" ));
    else
        cout << "Aucune informations sur les while conditions d'un �v�nement While";

    //Conditions
    if ( eventElem->FirstChildElement( "Conditions" ) != NULL )
        gd::EventsListSerialization::OpenConditions(project, conditions, eventElem->FirstChildElement( "Conditions" ));
    else
        cout << "Aucune informations sur les conditions d'un �v�nement";

    //Actions
    if ( eventElem->FirstChildElement( "Actions" ) != NULL )
        gd::EventsListSerialization::OpenActions(project, actions, eventElem->FirstChildElement( "Actions" ));
    else
        cout << "Aucune informations sur les actions d'un �v�nement";

    //Subevents
    if ( eventElem->FirstChildElement( "Events" ) != NULL )
        gd::EventsListSerialization::LoadEventsFromXml(project, events, eventElem->FirstChildElement( "Events" ));
}

/**
 * Render the event in the bitmap
 */
void WhileEvent::Render(wxDC & dc, int x, int y, unsigned int width, gd::EventsEditorItemsAreas & areas, gd::EventsEditorSelection & selection, const gd::Platform & platform)
{
    gd::EventsRenderingHelper * renderingHelper = gd::EventsRenderingHelper::GetInstance();
    int border = renderingHelper->instructionsListBorder;
    const int repeatHeight = 20;

    //Draw header rectangle
    int whileConditionsHeight = renderingHelper->GetRenderedConditionsListHeight(whileConditions, width-80-border*2, platform)+border*2;
    if (!infiniteLoopWarning && whileConditionsHeight < 32 ) whileConditionsHeight = 32;
    wxRect headerRect(x, y, width, whileConditionsHeight+repeatHeight);
    renderingHelper->DrawNiceRectangle(dc, headerRect);

    //While text
    dc.SetFont( renderingHelper->GetNiceFont().Bold()  );
    dc.SetTextForeground(wxColour(0,0,0));
    dc.DrawText( _("While :"), x+5, y+5 );

    //Draw icon if infinite loop warning is deactivated.
    if (!infiniteLoopWarning)
    {
        if ( gd::CommonBitmapManager::GetInstance()->noProtection.IsOk() )
            dc.DrawBitmap(gd::CommonBitmapManager::GetInstance()->noProtection, wxPoint(x+5,y+5+18), /*useMask=*/true);
    }

    //Draw "while conditions"
    renderingHelper->DrawConditionsList(whileConditions, dc, x+80+border, y+border, width-80-border*2, this, areas, selection, platform);

    dc.SetFont( renderingHelper->GetNiceFont().Bold()  );
    dc.SetTextForeground(wxColour(0,0,0));
    dc.DrawText( _("Repeat :"), x+4, y+whileConditionsHeight+3);
    whileConditionsHeight += repeatHeight;

    //Draw conditions rectangle
    wxRect rect(x, y+whileConditionsHeight, renderingHelper->GetConditionsColumnWidth()+border, GetRenderedHeight(width, platform)-whileConditionsHeight);
    renderingHelper->DrawNiceRectangle(dc, rect);

    renderingHelper->DrawConditionsList(conditions, dc,
                                        x+border,
                                        y+whileConditionsHeight+border,
                                        renderingHelper->GetConditionsColumnWidth()-border, this, areas, selection, platform);
    renderingHelper->DrawActionsList(actions, dc,
                                     x+renderingHelper->GetConditionsColumnWidth()+border,
                                     y+whileConditionsHeight+border,
                                     width-renderingHelper->GetConditionsColumnWidth()-border*2, this, areas, selection, platform);
}

unsigned int WhileEvent::GetRenderedHeight(unsigned int width, const gd::Platform & platform) const
{
    if ( eventHeightNeedUpdate )
    {
        gd::EventsRenderingHelper * renderingHelper = gd::EventsRenderingHelper::GetInstance();
        int border = renderingHelper->instructionsListBorder;
        const int repeatHeight = 20;

        //Get maximum height needed
        int whileConditionsHeight = renderingHelper->GetRenderedConditionsListHeight(whileConditions, width-80-border*2, platform);
        if (!infiniteLoopWarning && whileConditionsHeight < 32 ) whileConditionsHeight = 32;
        int conditionsHeight = renderingHelper->GetRenderedConditionsListHeight(conditions, renderingHelper->GetConditionsColumnWidth()-border, platform);
        int actionsHeight = renderingHelper->GetRenderedActionsListHeight(actions, width-renderingHelper->GetConditionsColumnWidth()-border*2, platform);

        renderedHeight = (( conditionsHeight > actionsHeight ? conditionsHeight : actionsHeight ) + whileConditionsHeight + repeatHeight)+border*2+border*2;
        eventHeightNeedUpdate = false;
    }

    return renderedHeight;
}

gd::BaseEvent::EditEventReturnType WhileEvent::EditEvent(wxWindow* parent_, gd::Project & game_, gd::Layout & scene_, gd::MainFrameWrapper & mainFrameWrapper_)
{
    if ( !justCreatedByTheUser )
    {
        wxMessageDialog dialog(parent_, _("Do you want to activate the display of a message if the event is repeated 100000 times? This message prevent the software from being frozen if you accidentally create an infinite loop ( by letting the conditions empty for example )."),
                               _("Protection against infinite loops."),
                               wxYES_NO|wxCANCEL|wxICON_INFORMATION);

        dialog.SetExtendedMessage(_("This message is activated by default for \"While\" events in the IDE, and is deactivated when the game is compiled to an executable."));
        dialog.SetYesNoCancelLabels(_("Activate"), _("Deactivate"), _("Cancel"));

        int answer = dialog.ShowModal();
        if ( answer == wxID_YES ) infiniteLoopWarning = true;
        else if ( answer == wxID_NO ) infiniteLoopWarning = false;
    }
    justCreatedByTheUser = false; //Show Message dialog next time.

    return ChangesMade;
}

/**
 * Initialize from another WhileEvent.
 * Used by copy ctor and assignement operator
 */
void WhileEvent::Init(const WhileEvent & event)
{
    events = CloneVectorOfEvents(event.events);

    whileConditions = event.whileConditions;
    conditions = event.conditions;
    actions = event.actions;
    infiniteLoopWarning = event.infiniteLoopWarning;
    justCreatedByTheUser = event.justCreatedByTheUser;
}

/**
 * Custom copy operator
 */
WhileEvent::WhileEvent(const WhileEvent & event) :
BaseEvent(event)
{
    Init(event);
}

/**
 * Custom assignement operator
 */
WhileEvent& WhileEvent::operator=(const WhileEvent & event)
{
    if ( this != &event )
    {
        BaseEvent::operator=(event);
        Init(event);
    }

    return *this;
}


}