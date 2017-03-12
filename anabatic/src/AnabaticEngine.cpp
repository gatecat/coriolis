// -*- C++ -*-
//
// This file is part of the Coriolis Software.
// Copyright (c) UPMC 2016-2016, All Rights Reserved
//
// +-----------------------------------------------------------------+
// |                   C O R I O L I S                               |
// |     A n a b a t i c  -  Global Routing Toolbox                  |
// |                                                                 |
// |  Author      :                    Jean-Paul CHAPUT              |
// |  E-mail      :            Jean-Paul.Chaput@lip6.fr              |
// | =============================================================== |
// |  C++ Module  :  "./AnabaticEngine.cpp"                          |
// +-----------------------------------------------------------------+


#include <sstream>
#include <iostream>
#include "hurricane/Bug.h"
#include "hurricane/Error.h"
#include "hurricane/Breakpoint.h"
#include "hurricane/RegularLayer.h"
#include "hurricane/Horizontal.h"
#include "hurricane/RoutingPad.h"
#include "hurricane/Vertical.h"
#include "hurricane/Cell.h"
#include "hurricane/DebugSession.h"
#include "hurricane/UpdateSession.h"
#include "crlcore/RoutingGauge.h"
#include "anabatic/GCell.h"
#include "anabatic/AnabaticEngine.h"


namespace Anabatic {

  using std::cerr;
  using std::cout;
  using std::endl;
  using std::ostringstream;
  using Hurricane::Bug;
  using Hurricane::Error;
  using Hurricane::Breakpoint;
  using Hurricane::RegularLayer;
  using Hurricane::Component;
  using Hurricane::Horizontal;
  using Hurricane::Vertical;
  using Hurricane::NetRoutingExtension;
  using Hurricane::Cell;
  using Hurricane::DebugSession;
  using Hurricane::UpdateSession;
  using CRL::RoutingGauge;
  using CRL::RoutingLayerGauge;


// -------------------------------------------------------------------
// Error messages.

  const char* missingAnbt =
    "%s :\n\n"
    "    Cell %s do not have any Anabatic (or not yet created).\n";

  const char* badMethod =
    "%s :\n\n"
    "    No method id %ud (Cell %s).\n";

  const char* lookupFailed =
    "Anabatic::Extension::getDatas(Segment*) :\n\n"
    "    Cannot find AutoSegment associated to %s (internal error).\n";


// -------------------------------------------------------------------
// Class  :  "Anabatic::RawGCellsUnder".

  RawGCellsUnder::RawGCellsUnder ( const AnabaticEngine* engine, Segment* segment )
  {
    cdebug_log(112,1) << "RawGCellsUnder::RawGCellsUnder(): " << segment << endl;

    Box   gcellsArea     = engine->getCell()->getAbutmentBox();
    Point sourcePosition = segment->getSourcePosition();
    Point targetPosition = segment->getTargetPosition();

    if (  (sourcePosition.getX() >  gcellsArea.getXMax())
       or (sourcePosition.getY() >  gcellsArea.getYMax())
       or (targetPosition.getX() <= gcellsArea.getXMin())
       or (targetPosition.getY() <= gcellsArea.getYMin()) ) {
      cerr << Error( "RawGCellsUnder::RawGCellsUnder(): %s is completly outside the GCells area (ignored)."
                   , getString(segment).c_str()
                   ) << endl;
      cdebug_tabw(112,-1);
      DebugSession::close();
      return;
    }

    DbU::Unit xsource = std::max( sourcePosition.getX(), gcellsArea.getXMin() );
    DbU::Unit ysource = std::max( sourcePosition.getY(), gcellsArea.getYMin() );
    DbU::Unit xtarget = std::min( targetPosition.getX(), gcellsArea.getXMax() );
    DbU::Unit ytarget = std::min( targetPosition.getY(), gcellsArea.getYMax() );

    if (xtarget == gcellsArea.getXMax()) --xtarget;
    if (ytarget == gcellsArea.getYMax()) --ytarget;

    GCell* gsource = engine->getGCellUnder( xsource, ysource );
    GCell* gtarget = engine->getGCellUnder( xtarget, ytarget );

    if (not gsource) {
      cerr << Bug( "RawGCellsUnder::RawGCellsUnder(): %s source not under a GCell (ignored)."
                 , getString(segment).c_str()
                 ) << endl;
      cdebug_tabw(112,-1);
      DebugSession::close();
      return;
    }
    if (not gtarget) {
      cerr << Bug( "RawGCellsUnder::RawGCellsUnder(): %s target not under a GCell (ignored)."
                 , getString(segment).c_str()
                 ) << endl;
      cdebug_tabw(112,-1);
      DebugSession::close();
      return;
    }

    if (gsource == gtarget) {
      _elements.push_back( Element(gsource,NULL) );
      cdebug_tabw(112,-1);
      DebugSession::close();
      return;
    }

    Flags       side       = Flags::NoFlags;
    DbU::Unit   axis       = 0;
    Horizontal* horizontal = dynamic_cast<Horizontal*>( segment );
    if (horizontal) {
      side = Flags::EastSide;
      axis = horizontal->getY();

      if (horizontal->getSourceX() > horizontal->getTargetX())
        std::swap( gsource, gtarget );
    } else {
      Vertical* vertical = dynamic_cast<Vertical*>( segment );
      side = Flags::NorthSide;
      axis = vertical->getX();

      if (vertical->getSourceY() > vertical->getTargetY())
        std::swap( gsource, gtarget );
    }

    Edge* edge = gsource->getEdgeAt( side, axis );
    while ( edge ) {
      _elements.push_back( Element(edge->getSource(),edge) );

      if (edge->getTarget() == gtarget) break;
      edge = edge->getTarget()->getEdgeAt( side, axis );
    } 
    _elements.push_back( Element(gtarget,NULL) );

    cdebug_tabw(112,-1);
  }


// -------------------------------------------------------------------
// Class  :  "Anabatic::NetData".

  NetData::NetData ( Net* net )
    : _net       (net)
    , _state     (NetRoutingExtension::get(net))
    , _searchArea()
    , _rpCount   (0)
    , _sparsity  (0)
    , _flags     ()
  {
    if (_state and _state->isMixedPreRoute()) return;

    for ( RoutingPad* rp : _net->getRoutingPads() ) {
      _searchArea.merge( rp->getBoundingBox() );
      ++_rpCount;
    }
    _update();
  }


// -------------------------------------------------------------------
// Class  :  "Anabatic::AnabaticEngine".

  Name  AnabaticEngine::_toolName = "Anabatic";


  AnabaticEngine* AnabaticEngine::get ( const Cell* cell )
  { return static_cast<AnabaticEngine*>(ToolEngine::get(cell,staticGetName())); }


  const Name& AnabaticEngine::staticGetName ()
  { return _toolName; }


  const Name& AnabaticEngine::getName () const
  { return _toolName; }


  AnabaticEngine::AnabaticEngine ( Cell* cell )
    : Super(cell)
    , _timer           ()
    , _configuration   (new Configuration())
    , _chipTools       (cell)
    , _state           (EngineCreation)
    , _matrix          ()
    , _gcells          ()
    , _ovEdges         ()
    , _netOrdering     ()
    , _netDatas        ()
    , _viewer          (NULL)
    , _flags           (Flags::DestroyBaseContact)
    , _stamp           (-1)
    , _densityMode     (MaxDensity)
    , _autoSegmentLut  ()
    , _autoContactLut  ()
    , _blockageNet     (cell->getNet("blockagenet"))
  {
    _matrix.setCell( cell, _configuration->getSliceHeight() );
    Edge::unity = _configuration->getSliceHeight();

    if (not _blockageNet) _blockageNet = Net::create( cell, "blockagenet" );
  }


  void  AnabaticEngine::_postCreate ()
  {
    Super::_postCreate();

    UpdateSession::open();
    GCell::create( this );
    UpdateSession::close();
  }


  AnabaticEngine* AnabaticEngine::create ( Cell* cell )
  {
    if (not cell) throw Error( "AnabaticEngine::create(): NULL cell argument." );
    if (cell->getAbutmentBox().isEmpty())
      throw Error( "AnabaticEngine::create(): %s has no abutment box." , getString(cell).c_str() );
    
    AnabaticEngine* engine = new AnabaticEngine ( cell );
    engine->_postCreate();
    return engine;
  }


  AnabaticEngine::~AnabaticEngine ()
  {
    delete _configuration;
    for ( pair<unsigned int,NetData*> data : _netDatas ) delete data.second;
  }


  void  AnabaticEngine::_preDestroy ()
  {
    cdebug_log(145,1) << "Anabatic::_preDestroy ()" << endl;

    if (getState() < EngineGutted)
      setState( EnginePreDestroying );

    _gutAnabatic();
    _state = EngineGutted;

    cdebug_log(145,0) << "About to delete base class ToolEngine." << endl;
    Super::_preDestroy();

  //cmess2 << "     - GCells        := " << GCell::getAllocateds() << endl;
    cmess2 << "     - AutoContacts  := " << AutoContact::getAllocateds() << endl;
    cmess2 << "     - AutoSegments  := " << AutoSegment::getAllocateds() << endl;

    cdebug_log(145,0) << "Exiting Anabatic::_preDestroy()." << endl;
    cdebug_tabw(145,-1);
  }


  void  AnabaticEngine::_gutAnabatic ()
  {
    openSession();

    _flags.reset( Flags::DestroyBaseContact|Flags::DestroyBaseSegment );

    if (_state == EngineDriving) {
      cdebug_log(145,1) << "Saving AutoContacts/AutoSegments." << endl;

      size_t fixedSegments    = 0;
      size_t sameLayerDoglegs = 0;
      for ( auto isegment : _autoSegmentLut ) {
        if (isegment.second->isFixed()) ++fixedSegments;
        if (isegment.second->reduceDoglegLayer()) ++sameLayerDoglegs;
      }

      cmess1 << "  o  Driving Hurricane data-base." << endl;
      cmess1 << Dots::asSizet("     - Active AutoSegments",AutoSegment::getAllocateds()-fixedSegments) << endl;
      cmess1 << Dots::asSizet("     - Active AutoContacts",AutoContact::getAllocateds()-fixedSegments*2) << endl;
      cmess1 << Dots::asSizet("     - AutoSegments"       ,AutoSegment::getAllocateds()) << endl;
      cmess1 << Dots::asSizet("     - AutoContacts"       ,AutoContact::getAllocateds()) << endl;
      cmess1 << Dots::asSizet("     - Same Layer doglegs" ,sameLayerDoglegs) << endl;

    //for ( Net* net : _cell->getNets() ) _saveNet( net );

      cdebug_tabw(145,-1);
    }

    if (_state < EngineGutted ) {
      cdebug_log(145,0) << "Gutting Anabatic." << endl;
      _state = EngineGutted;
      _flags |= Flags::DestroyBaseContact;

      _destroyAutoSegments();
      _destroyAutoContacts();

      _flags |= Flags::DestroyGCell;

      for ( GCell* gcell : _gcells ) gcell->_destroyEdges();
      for ( GCell* gcell : _gcells ) gcell->destroy();
      _gcells.clear();
      _ovEdges.clear();
    }

    Session::close();
  }


  Configuration* AnabaticEngine::getConfiguration ()
  { return _configuration; }


  Interval  AnabaticEngine::getUSide ( Flags direction ) const
  {
    Interval side;
    Box      bBox ( getCell()->getBoundingBox() );

    if      (direction & Flags::Horizontal) side = Interval( bBox.getXMin(), bBox.getXMax() );
    else if (direction & Flags::Vertical  ) side = Interval( bBox.getYMin(), bBox.getYMax() );
    else {
      cerr << Error( "AnabaticEngine::getUSide(): Unknown direction flag \"%i\""
                   , getString(direction).c_str() ) << endl;
    }
    return side;
  }


  int  AnabaticEngine::getCapacity ( Interval span, Flags flags ) const
  {
    int           capacity = 0;
    Box           ab       = getCell()->getAbutmentBox();
    RoutingGauge* rg       = _configuration->getRoutingGauge();

    span.inflate( 0, -1 );
    if (span.isEmpty()) return 0;

    const vector<RoutingLayerGauge*>& layerGauges = rg->getLayerGauges();
    for ( size_t depth=0 ; depth <= _configuration->getAllowedDepth() ; ++depth ) {
      if (layerGauges[depth]->getType() != Constant::Default) continue;

      if (flags & Flags::Horizontal) {
        if (layerGauges[depth]->getDirection() != Constant::Horizontal) continue;
        capacity += layerGauges[depth]->getTrackNumber( span.getVMin() - ab.getYMin()
                                                      , span.getVMax() - ab.getYMin() );
      //cdebug_log(110,0) << "Horizontal edge capacity:" << capacity << endl;
      }

      if (flags & Flags::Vertical) {
        if (layerGauges[depth]->getDirection() != Constant::Vertical) continue;
        capacity += layerGauges[depth]->getTrackNumber( span.getVMin() - ab.getXMin()
                                                      , span.getVMax() - ab.getXMin() );
      //cdebug_log(110,0) << "Vertical edge capacity:" << capacity << endl;
      }
    }

    return capacity;
  }


  void  AnabaticEngine::openSession ()
  { Session::_open(this); }


  void  AnabaticEngine::reset ()
  {
    _gutAnabatic();
    _flags.reset( Flags::DestroyMask );
    _state = EngineCreation;

    UpdateSession::open();
    GCell::create( this );
    UpdateSession::close();
  }


  void  AnabaticEngine::setupNetDatas ()
  {
    size_t  oindex = _netOrdering.size();
    for ( Net* net : _cell->getNets() ) {
      if (_netDatas.find(net->getId()) != _netDatas.end()) continue;
      _netOrdering.push_back( new NetData(net) );
    }

    for ( ; oindex < _netOrdering.size() ; ++oindex ) {
      _netDatas.insert( make_pair( _netOrdering[oindex]->getNet()->getId()
                                 , _netOrdering[oindex] ) );
    }

    sort( _netOrdering.begin(), _netOrdering.end(), SparsityOrder() );
  }


  void AnabaticEngine::updateMatrix()
  {
    _matrix.setCell( getCell(), Session::getSliceHeight() );
    for ( GCell* gcell : _gcells ){
      gcell->_revalidate();
    }
  }

  size_t  AnabaticEngine::getNetsFromEdge ( const Edge* edge, NetSet& nets )
  {
    size_t  count  = 0;
    GCell*  source = edge->getSource();
    GCell*  target = edge->getTarget();
    const vector<Contact*>& contacts = source->getGContacts();

    for ( Contact* contact : contacts ) {
      for ( Component* component : contact->getSlaveComponents() ) {
        if (edge->isHorizontal()) {
          Horizontal* horizontal = dynamic_cast<Horizontal*>( component );
          if (horizontal
             and (horizontal->getSource() == contact)
             and (target->hasGContact(dynamic_cast<Contact*>(horizontal->getTarget())))) {
            nets.insert( horizontal->getNet() );
            ++count;
          }
        }
        if (edge->isVertical()) {
          Vertical* vertical = dynamic_cast<Vertical*>( component );
          if (vertical
             and (vertical->getSource() == contact)
             and (target->hasGContact(dynamic_cast<Contact*>(vertical->getTarget())))) {
            nets.insert( vertical->getNet() );
            ++count;
          }
        }
      }
    }
    return count;
  }


  NetData* AnabaticEngine::getNetData ( Net* net, unsigned int flags )
  {
    NetData*            data = NULL;
    NetDatas::iterator idata = _netDatas.find( net->getId() );
    if (idata == _netDatas.end()) {
      data = new NetData( net );
      _netDatas.insert( make_pair(net->getId(),data) );
      _netOrdering.push_back( data );
      // cerr << Bug( "AnabaticEngine::getNetData() - %s is missing in NetDatas table."
      //            , getString(net->getName()).c_str()
      //            ) << endl;
      // return NULL;
    } else
      data = idata->second;

    if ((flags & Flags::Create) and not data->getNetRoutingState()) {
      data->setNetRoutingState( NetRoutingExtension::create(net) );
    }

    return data;
  }


  Contact* AnabaticEngine::breakAt ( Segment* segment, GCell* breakGCell )
  {
    size_t       i      = 0;
    GCellsUnder  gcells ( new RawGCellsUnder(this,segment) );
    for ( ; i<gcells->size() ; ++i ) {
      if (gcells->gcellAt(i) == breakGCell) break;
    }

    Contact* breakContact = breakGCell->getGContact( segment->getNet() );

    if (i == gcells->size()) {
      cerr << Error( "AnabaticEngine::breakAt(): %s is *not* over %s."
                   , getString(segment).c_str()
                   , getString(breakGCell).c_str()
                   ) << endl;
      return breakContact;
    }

    Component* targetContact = segment->getTarget();
    segment->getTargetHook()->detach();
    segment->getTargetHook()->attach( breakContact->getBodyHook() );

    Segment*    splitted   = NULL;
    Horizontal* horizontal = dynamic_cast<Horizontal*>(segment);
    if (horizontal) {
      splitted = Horizontal::create( breakContact
                                   , targetContact
                                   , getConfiguration()->getGHorizontalLayer()
                                   , horizontal->getY()
                                   , DbU::fromLambda(2.0)
                                   );
    } else {
      Vertical* vertical = dynamic_cast<Vertical*>(segment);
      if (vertical) {
        splitted = Vertical::create( breakContact
                                   , targetContact
                                   , getConfiguration()->getGVerticalLayer()
                                   , vertical->getX()
                                   , DbU::fromLambda(2.0)
                                   );
      } else
        return breakContact;
    }

    for ( ; i<gcells->size()-1 ; ++i ) gcells->edgeAt(i)->replace( segment, splitted );

    return breakContact;
  }


  bool  AnabaticEngine::unify ( Contact* contact )
  {
    size_t      hCount     = 0;
    size_t      vCount     = 0;
    Horizontal* horizontals[2];
    Vertical*   verticals  [2];

    for ( Component* slave : contact->getSlaveComponents() ) {
      Horizontal* h = dynamic_cast<Horizontal*>( slave );
      if (h) {
        if (vCount or (hCount > 1)) return false;
        horizontals[hCount++] = h;
      } else {
        Vertical* v = dynamic_cast<Vertical*>( slave );
        if (v) {
          if (hCount or (vCount > 1)) return false;
          verticals[vCount++] = v;
        } else {
        // Something else depends on this contact.
          return false;
        }
      }
    }

    if (hCount == 2) {
      if (horizontals[0]->getTarget() != contact) std::swap( horizontals[0], horizontals[1] );
      Interval    constraints ( false );
      GCellsUnder gcells0     = getGCellsUnder( horizontals[0] );
      if (not gcells0->empty()) {
        for ( size_t i=0 ; i<gcells0->size() ; ++i )
          constraints.intersection( gcells0->gcellAt(i)->getSide(Flags::Vertical) );
      }

      GCellsUnder gcells1 = getGCellsUnder( horizontals[1] );
      if (not gcells1->empty()) {
        for ( size_t i=0 ; i<gcells1->size() ; ++i ) {
          constraints.intersection( gcells1->gcellAt(i)->getSide(Flags::Vertical) );
          if (constraints.isEmpty()) return false;
        }
      }

      if (not gcells1->empty()) {
        for ( size_t i=0 ; i<gcells1->size()-1 ; ++i )
          gcells1->edgeAt(i)->replace( horizontals[1], horizontals[0] );
      }

      Component* target = horizontals[1]->getTarget();
      horizontals[1]->destroy();
      horizontals[0]->getTargetHook()->detach();
      horizontals[0]->getTargetHook()->attach( target->getBodyHook() );
    } 

    if (vCount == 2) {
      if (verticals[0]->getTarget() != contact) std::swap( verticals[0], verticals[1] );
      Interval    constraints ( false );
      GCellsUnder gcells0     = getGCellsUnder( verticals[0] );
      if (not gcells0->empty()) {
        for ( size_t i=0 ; i<gcells0->size() ; ++i )
          constraints.intersection( gcells0->gcellAt(i)->getSide(Flags::Horizontal) );
      }

      GCellsUnder gcells1 = getGCellsUnder( verticals[1] );
      if (not gcells1->empty()) {
        for ( size_t i=0 ; i<gcells1->size() ; ++i ) {
          constraints.intersection( gcells1->gcellAt(i)->getSide(Flags::Horizontal) );
          if (constraints.isEmpty()) return false;
        }
      }

      if (not gcells1->empty()) {
        for ( size_t i=0 ; i<gcells1->size()-1 ; ++i )
          gcells1->edgeAt(i)->replace( verticals[1], verticals[0] );
      }

      Component* target = verticals[1]->getTarget();
      verticals[1]->destroy();
      verticals[0]->getTargetHook()->detach();
      verticals[0]->getTargetHook()->attach( target->getBodyHook() );
    } 

    getGCellUnder( contact->getPosition() )->unrefContact( contact );

    return true;
  }


  void  AnabaticEngine::ripup ( Segment* seed, Flags flags )
  {

    Net* net = seed->getNet();

    DebugSession::open( net, 112, 120 );
    cdebug_log(112,1) << "AnabaticEngine::ripup(): " << seed << endl;

    Contact* end0 = NULL;
    Contact* end1 = NULL;

    vector<Segment*> ripups;
    ripups.push_back( seed );

    vector< pair<Segment*,Component*> > stack;
    if (flags & Flags::Propagate) {
      stack.push_back( make_pair(seed,seed->getSource()) );
      stack.push_back( make_pair(seed,seed->getTarget()) );
    }

    while ( not stack.empty() ) {
      Contact* contact = dynamic_cast<Contact*>( stack.back().second );
      Segment* from    = stack.back().first;
      stack.pop_back();
      if (not contact) continue;

      Segment* connected  = NULL;
      size_t   slaveCount = 0;
      for ( Hook* hook : contact->getBodyHook()->getHooks() ) {
        Component* linked = hook->getComponent();
        if ((linked == contact) or (linked == from)) continue;

        if (dynamic_cast<RoutingPad*>(linked)) { ++slaveCount; continue; }

        connected = dynamic_cast<Segment*>( linked );
        if (connected) ++slaveCount; 
      }

      if ((slaveCount == 1) and (connected)) {
        stack .push_back( make_pair(connected,connected->getOppositeAnchor(contact)) );
        ripups.push_back( connected );
      } else {
        if (not end0) {
          end0 = contact;
          cdebug_log(112,0) << "end0:" << contact << endl;
        } else {
          end1 = contact;
          cdebug_log(112,0) << "end1:" << contact << endl;
        }
      }
    }

    for ( Segment* segment : ripups ) {
      cdebug_log(112,1) << "| Destroy:" << segment << endl;

      GCellsUnder gcells = getGCellsUnder( segment );
      if (not gcells->empty()) {
        for ( size_t i=0 ; i<gcells->size()-1 ; ++i )
          gcells->edgeAt(i)->remove( segment );
      }

      Contact* source = dynamic_cast<Contact*>( segment->getSource() );
      Contact* target = dynamic_cast<Contact*>( segment->getTarget() );
      segment->getSourceHook()->detach();
      segment->getTargetHook()->detach();
      segment->destroy();
      bool deletedSource = gcells->gcellAt( 0                )->unrefContact( source );
      bool deletedTarget = gcells->gcellAt( gcells->size()-1 )->unrefContact( target );

      if (deletedSource) {
        if (source == end0) end0 = NULL;
        if (source == end1) end1 = NULL;
      }
      if (deletedTarget) {
        if (target == end0) end0 = NULL;
        if (target == end1) end1 = NULL;
      }

      cdebug_tabw(112,-1);
    }

    if (end0) unify( end0 );
    if (end1) unify( end1 );

    getNetData( net )->setGlobalRouted( false );

    cdebug_tabw(112,-1);
    DebugSession::close();
  }


  void  AnabaticEngine::cleanupGlobal ()
  {
    UpdateSession::open();
    for ( GCell* gcell : _gcells ) gcell->cleanupGlobal();
    UpdateSession::close();
  }


  void  AnabaticEngine::loadGlobalRouting ( unsigned int method )
  {
    if (_state < EngineGlobalLoaded)
      throw Error ("AnabaticEngine::loadGlobalRouting() : global routing not present yet.");

    if (_state > EngineGlobalLoaded)
      throw Error ("AnabaticEngine::loadGlobalRouting() : global routing already loaded.");

    switch ( method ) {
      case EngineLoadGrByNet:   _loadGrByNet(); break;
      case EngineLoadGrByGCell:
      default:
        throw Error( badMethod
                   , "Anabatic::loadGlobalRouting()"
                   , method
                   , getString(_cell).c_str()
                   );
    }
    cleanupGlobal();

    _state = EngineActive;
  }


  void  AnabaticEngine::updateNetTopology ( Net* net )
  {
    DebugSession::open( net, 140, 150 );

    cdebug_log(149,0) << "Anabatic::updateNetTopology( " << net << " )" << endl;
    cdebug_tabw(145,1);

    vector<AutoContact*>  contacts;
    for ( Component* component : net->getComponents() ) {
      Contact* contact = dynamic_cast<Contact*>( component );
      if (contact) {
        AutoContact* autoContact = Session::lookup( contact );
        if (autoContact and autoContact->isInvalidatedCache()) 
          contacts.push_back( autoContact );
      }
    }

    for ( size_t i=0 ; i<contacts.size() ; ++i )
      contacts[i]->updateTopology();

    cdebug_tabw(145,-1);
    DebugSession::close();
  }


  void  AnabaticEngine::finalizeLayout ()
  {
    cdebug_log(145,0) << "Anabatic::finalizeLayout()" << endl;
    if (_state > EngineDriving) return;

    _state = EngineDriving;

    startMeasures();
    _gutAnabatic();
    stopMeasures ();
    printMeasures( "fin" );

    _state = EngineGutted;
  }


  void  AnabaticEngine::_alignate ( Net* net )
  {
    DebugSession::open( net, 140, 150 );

    cdebug_log(149,0) << "Anabatic::_alignate( " << net << " )" << endl;
    cdebug_tabw(145,1);

  //cmess2 << "     - " << getString(net) << endl;

    set<Segment*>        exploredSegments;
    vector<AutoSegment*> unexploreds;
    vector<AutoSegment*> aligneds;

    for ( Component* component : net->getComponents() ) {
      Segment* segment = dynamic_cast<Segment*>(component);
      if (segment) {
        AutoSegment* seedSegment = Session::lookup( segment );
        if (seedSegment) unexploreds.push_back( seedSegment );
      }
    }
    sort( unexploreds.begin(), unexploreds.end(), AutoSegment::CompareId() );

    for ( size_t i=0 ; i<unexploreds.size() ; i++ ) {
      AutoSegment* seedSegment = unexploreds[i];

      if (exploredSegments.find(seedSegment->base()) == exploredSegments.end()) {
        cdebug_log(145,0) << "New chunk from: " << seedSegment << endl;
        aligneds.push_back( seedSegment );

        for ( AutoSegment* collapsed : seedSegment->getAligneds() ) {
          cdebug_log(145,0) << "Aligned: " << collapsed << endl;
          aligneds.push_back( collapsed );
          exploredSegments.insert( collapsed->base() );
        }

        cdebug_tabw(145,1);
        sort( aligneds.begin(), aligneds.end(), AutoSegment::CompareId() );

        cdebug_log(145,0) << "Seed: " << (void*)aligneds[0]->base() << " " << aligneds[0] << endl;
        for ( size_t j=1 ; j<aligneds.size() ; j++ ) {
          cdebug_log(145,0) << "Secondary: " << (void*)(aligneds[j]->base()) << " " << aligneds[j] << endl;
        }

        cdebug_log(149,0) << "Align on " << aligneds[0]
                    << " " << DbU::getValueString(aligneds[0]->getAxis()) << endl;
        aligneds[0]->setAxis( aligneds[0]->getAxis(), Flags::Realignate );
        aligneds.clear();

        cdebug_tabw(145,-1);
      }
    }

    cdebug_tabw(145,-1);

    DebugSession::close();
  }


  void  AnabaticEngine::_computeNetTerminals ( Net* net )
  {
    DebugSession::open( net, 140, 150 );

    cdebug_log(149,0) << "Anabatic::_computeNetTerminals( " << net << " )" << endl;
    cdebug_tabw(145,1);

    for ( Segment* segment : net->getSegments() ) {
      AutoSegment* autoSegment = Session::lookup( segment );
      if (autoSegment == NULL) continue;
      if (autoSegment->isInvalidated()) autoSegment->computeTerminal();
    }

    cdebug_tabw(145,-1);

    DebugSession::close();
  }


  void  AnabaticEngine::_saveNet ( Net* net )
  {
    DebugSession::open( net, 140, 150 );

    cdebug_log(145,0) << "Anabatic::_saveNet() " << net << endl;
    cdebug_tabw(145,1);

#if 0
    cdebug_log(145,0) << "Deleting zero-length segments." << endl;

    vector<Segment*>   nullSegments;
    set<const Layer*>  connectedLayers;

    forEach ( Segment*, segment, net->getSegments() ) {
      if (segment->getLength()) {
        if (net->isExternal()) {
          NetExternalComponents::setExternal( *segment );
        }
        continue;
      }
    
      if (Session::lookup(*segment) == NULL) {
        cdebug_log(145,0) << "* Not associated to an AutoSegment: " << *segment << endl;
        continue;
      }

      if (not isTopAndBottomConnected(*segment,connectedLayers)) {
        nullSegments.push_back( *segment );
        cdebug_log(145,0) << "* Null Length: " << *segment << endl;
      }
    }
    
    setFlags( EngineDestroyBaseSegment );
    for ( size_t i = 0 ; i < nullSegments.size() ; i++ ) {
      Contact* source = dynamic_cast<Contact*>(nullSegments[i]->getSource());
      Contact* target = dynamic_cast<Contact*>(nullSegments[i]->getTarget());

      if ( (source == NULL) or (target == NULL) ) {
        cerr << Error("Unconnected source/target on %s.",getString(nullSegments[i]).c_str()) << endl;
        continue;
      }

      if (source->getAnchor()) {
        if (target->getAnchor()) {
          continue;
        //cerr << Bug("Both source & target are anchored while deleting zero-length segment:\n"
        //            "       %s.",getString(nullSegments[i]).c_str()) << endl;
        } else
          swap( source, target );
      }

      cdebug_log(145,0) << "Deleting: " << nullSegments[i] << endl;
      if (isTopAndBottomConnected(nullSegments[i],connectedLayers)) {
        cdebug_log(145,0) << "Deletion cancelled, no longer top or bottom connected." << endl;
        continue;
      }

      cdebug_log(145,0) << "* Source: " << (void*)source << " " << source << endl;
      cdebug_log(145,0) << "* Target: " << (void*)target << " " << target << endl;

      const Layer* layer = DataBase::getDB()->getTechnology()
        ->getViaBetween( *connectedLayers.begin(), *connectedLayers.rbegin() );

      cdebug_log(145,0) << *connectedLayers.begin() << " + " << *connectedLayers.rbegin() << endl;
      cdebug_log(145,0) << "* Shrink layer: " << layer << endl;
      if ( !layer ) {
        cerr << Error("NULL contact layer while deleting %s."
                     ,getString(nullSegments[i]).c_str()) << endl;
        continue;
      }

      Session::lookup( nullSegments[i] )->destroy ();

      vector<Hook*>  slaveHooks;
      Hook*          masterHook = source->getBodyHook()->getPreviousMasterHook();

      while ( masterHook->getNextHook() != source->getBodyHook() ) {
        slaveHooks.push_back( masterHook->getNextHook() );
        cdebug_log(145,0) << "* detach: "
                   << (void*)masterHook->getNextHook()->getComponent()
                   << " " << masterHook->getNextHook()->getComponent() << endl;
        masterHook->getNextHook()->detach();
      }
      source->destroy();

      masterHook = target->getBodyHook();
      for ( size_t j=0 ; j < slaveHooks.size() ; j++ ) {
        slaveHooks[j]->attach( masterHook );
      }

      cdebug_log(145,0) << (void*)target << " " << target << " setLayer: " << layer << endl;
      target->setLayer( layer );
    }
    unsetFlags( EngineDestroyBaseSegment );
#endif

    cdebug_tabw(145,-1);
    DebugSession::close();
  }


  void  AnabaticEngine::startMeasures ()
  {
    _timer.resetIncrease();
    _timer.start();
  }


  void  AnabaticEngine::stopMeasures ()
  { _timer.stop(); }


  void  AnabaticEngine::suspendMeasures ()
  { _timer.suspend(); }


  void  AnabaticEngine::resumeMeasures ()
  { _timer.resume(); }


  void  AnabaticEngine::printMeasures ( const string& tag ) const
  {
    ostringstream result;

    result <<  Timer::getStringTime(_timer.getCombTime()) 
           << ", " << Timer::getStringMemory(_timer.getIncrease());
    cmess1 << Dots::asString( "     - Done in", result.str() ) << endl;

    result.str("");
    result << _timer.getCombTime()
           << "s, +" << (_timer.getIncrease()>>10) <<  "Kb/"
           <<  Timer::getStringMemory(Timer::getMemorySize());
    cmess2 << Dots::asString( "     - Raw measurements", result.str() ) << endl;

    // result.str("");
    // result << Timer::getStringMemory(Timer::getMemorySize());
    // cmess1 << Dots::asString( "     - Total memory", result.str() ) << endl;
  }


  void  AnabaticEngine::updateDensity ()
  { for ( GCell* gcell : _gcells ) gcell->updateDensity(); } 


  size_t  AnabaticEngine::checkGCellDensities ()
  {
    size_t saturateds = 0;
    for ( GCell* gcell : _gcells ) saturateds += gcell->checkDensity();
    return saturateds;
  } 


  AutoSegment* AnabaticEngine::_lookup ( Segment* segment ) const
  {
    AutoSegmentLut::const_iterator it = _autoSegmentLut.find( segment );
    if (it == _autoSegmentLut.end()) return NULL;

    return (*it).second;
  }


  void  AnabaticEngine::_link ( AutoSegment* autoSegment )
  {
    if (_state > EngineActive) return;
    _autoSegmentLut[ autoSegment->base() ] = autoSegment;
  }


  void  AnabaticEngine::_unlink ( AutoSegment* autoSegment )
  {
    if (_state > EngineDriving) return;

    AutoSegmentLut::iterator it = _autoSegmentLut.find( autoSegment->base() );
    if (it != _autoSegmentLut.end())
      _autoSegmentLut.erase( it );
  }


  AutoContact* AnabaticEngine::_lookup ( Contact* contact ) const
  {
    AutoContactLut::const_iterator it = _autoContactLut.find( contact );
    if (it == _autoContactLut.end()) {
      return NULL;
    }
    return (*it).second;
  }


  void  AnabaticEngine::_link ( AutoContact* autoContact )
  {
    if (_state > EngineActive) return;
    _autoContactLut [ autoContact->base() ] = autoContact;
  }


  void  AnabaticEngine::_unlink ( AutoContact* autoContact )
  {
    if ( _state > EngineActive ) return;

    AutoContactLut::iterator it = _autoContactLut.find( autoContact->base() );
    if (it != _autoContactLut.end())
      _autoContactLut.erase( it );
  }


  void  AnabaticEngine::_destroyAutoSegments ()
  {
    cdebug_log(145,0) << "Anabatic::_destroyAutoSegments ()" << endl;

    size_t expandeds = 0;
    for ( auto sasp : _autoSegmentLut ) {
      expandeds++;
      sasp.second->destroy();
    }
    if (_state == EngineDriving)
      cmess2 << "     - Expandeds     := " << expandeds << endl;

    _autoSegmentLut.clear();
  }


  void  AnabaticEngine::_destroyAutoContacts ()
  {
    cdebug_log(145,0) << "Anabatic::_destroyAutoContacts ()" << endl;

    for ( auto cacp : _autoContactLut ) cacp.second->destroy();
    _autoContactLut.clear();
  }


  void  AnabaticEngine::_check ( Net* net ) const
  {
    cdebug_log(149,1) << "Checking " << net << endl;
    for ( Segment* segment : net->getComponents().getSubSet<Segment*>() ) {
      AutoSegment* autoSegment = _lookup( segment );
      cdebug_log(149,0) << autoSegment << endl;
      if (autoSegment) {
        AutoContact* autoContact = autoSegment->getAutoSource();
        cdebug_log(149,0) << autoContact << endl;
        if (autoContact) autoContact->checkTopology();

        autoContact = autoSegment->getAutoTarget();
        cdebug_log(149,0) << autoContact << endl;
        if (autoContact) autoContact->checkTopology();
      }
    }
    cdebug_tabw(149,-1);
  }


  bool  AnabaticEngine::_check ( const char* message ) const
  {
    bool coherency = true;
    if (message)
      cerr << "     o  checking Anabatic DB (" << message << ")." << endl;

    for ( auto element : _autoSegmentLut )
      coherency = element.second->_check() and coherency;

    for ( GCell* gcell : _gcells ) {
      for ( AutoContact* contact : gcell->getContacts() )
        contact->checkTopology();
    }

    if (message) cerr << "        - completed." << endl;

    return coherency;
  }


  string  AnabaticEngine::_getTypeName () const
  { return getString(_toolName); }


  string  AnabaticEngine::_getString () const
  {
    ostringstream os;
    os << "<" << _toolName << " " << _cell->getName() << ">";
    return os.str();
  }


  Record* AnabaticEngine::_getRecord () const
  {
    Record* record = Super::_getRecord();
    record->add( getSlot("_configuration",  _configuration) );
    record->add( getSlot("_gcells"       , &_gcells       ) );
    record->add( getSlot("_matrix"       , &_matrix       ) );
    record->add( getSlot("_flags"        , &_flags        ) );
    return record;
  }


}  // Anabatic namespace.