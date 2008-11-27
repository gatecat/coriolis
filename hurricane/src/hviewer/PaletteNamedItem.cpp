
// -*- C++ -*-
//
// This file is part of the Coriolis Software.
// Copyright (c) UPMC/LIP6 2008-2008, All Rights Reserved
//
// ===================================================================
//
// $Id$
//
// x-----------------------------------------------------------------x 
// |                                                                 |
// |                  H U R R I C A N E                              |
// |     V L S I   B a c k e n d   D a t a - B a s e                 |
// |                                                                 |
// |  Author      :                    Jean-Paul CHAPUT              |
// |  E-mail      :       Jean-Paul.Chaput@asim.lip6.fr              |
// | =============================================================== |
// |  C++ Module  :       "./PaletteNamedItem.cpp"                   |
// | *************************************************************** |
// |  U p d a t e s                                                  |
// |                                                                 |
// x-----------------------------------------------------------------x


#include  <QCheckBox>
#include  <QHBoxLayout>

#include  "hurricane/viewer/Graphics.h"
#include  "hurricane/viewer/PaletteNamedItem.h"


namespace Hurricane {


  PaletteNamedItem::PaletteNamedItem ( const Name& name, bool checked )
    : PaletteItem()
    , _name(name)
  {
    QHBoxLayout* layout = new QHBoxLayout ();
    layout->setContentsMargins ( 0, 0, 0, 0 );

    _checkBox = new QCheckBox ( this );
    _checkBox->setChecked ( checked );
    _checkBox->setText    ( getString(getName()).c_str() );
    _checkBox->setFont    ( Graphics::getFixedFont() );
    layout->addWidget ( _checkBox );

    setLayout ( layout );

    connect ( _checkBox, SIGNAL(clicked()), this, SIGNAL(toggled()) );
  }


  PaletteNamedItem* PaletteNamedItem::create ( const Name& name, bool checked )
  {
    PaletteNamedItem* item = new PaletteNamedItem ( name, checked );
    return item;
  }


  const Name& PaletteNamedItem::getName () const
  {
    return _name;
  }


  bool  PaletteNamedItem::isChecked () const
  {
    return _checkBox->isChecked ();
  }


  void  PaletteNamedItem::setChecked ( bool state )
  {
    _checkBox->setChecked ( state );
  }


} // End of Hurricane namespace.