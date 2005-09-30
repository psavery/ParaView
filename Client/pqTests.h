/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqTests_h
#define _pqTests_h

class QWidget;

/// Given a root widget, returns a child widget by name (treating the input string as a hierarchical "path")
QWidget* pqLookupWidget(QWidget& Widget, const char* Name);

/// Runs regression tests that do not require a UI
void pqRunRegressionTests();
/// Runs regression tests that require a UI
void pqRunRegressionTests(QWidget& RootWidget);

#include <QtCore/QObject>

class pqTestTestingFramework :
  public QObject
{
  Q_OBJECT

private slots:
  void testSuccess();
  void testFailure();
};

class pqTestFileMenu :
  public QObject
{
  Q_OBJECT
  
public:
  pqTestFileMenu(QWidget& RootWidget) :
    rootWidget(RootWidget)
  {
  }

private:
  QWidget& rootWidget;

private slots:
  void testFileMenu();
  void testFileOpen();
};

#endif // !_pqTests_h

