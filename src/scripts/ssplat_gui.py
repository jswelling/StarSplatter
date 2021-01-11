#! /usr/bin/env python
#
#/****************************************************************************
# * ssplat_gui.py
# * Author Joel Welling
# * Copyright 2005, Pittsburgh Supercomputing Center, Carnegie Mellon University
# *
# * Permission to use, copy, and modify this software and its documentation
# * without fee for personal use or use within your organization is hereby
# * granted, provided that the above copyright notice is preserved in all
# * copies and that that copyright and this permission notice appear in
# * supporting documentation.  Permission to redistribute this software to
# * other organizations or individuals is not granted;  that must be
# * negotiated with the PSC.  Neither the PSC nor Carnegie Mellon
# * University make any representations about the suitability of this
# * software for any purpose.  It is provided "as is" without express or
# * implied warranty.
# *****************************************************************************/
# Notes:
# -property ranges should be available, to initialize cmap limits
#
#

import sys
import os
import os.path
import string
import getopt
import re
import time
import math

try:
    import starsplatter
except Exception as e:
    print("Can't import starsplatter: %s" % e)
    sys.exit(-1)

try:
    from wxPython.wx import *
except Exception as e:
    print("Can't import wxPython.wx: %s" % e)
    sys.exit(-1)

verboseFlag= 0
debugFlag= 0
imageDims= (640,480)
#imageDims= (64,64)

def List_to_String(list):
    text=''
    for i in range(len(list)):
        text=text + list[i]
    return text

def setDefaultRenderingAttributes( bunch ):
    bunch.set_bunch_color((0.8, 0.5, 1.0, 1.0))
    bunch.set_density(0.00002)
    bunch.set_exp_constant(1000.0)

def createDefaultCamera():
    mycam= starsplatter.Camera((-5.0,-9.0,3.0),
                               (0.0,0.0,0.0),
                               (0.0,0.0,1.0),
                               35.0,-1.0,-50.0)
    return mycam

def createBBoxCamera(bbox):
    atPt= bbox.center()
    d= bbox.xmax()-bbox.xmin()
    diagSqr= d*d
    d= bbox.ymax()-bbox.ymin()
    diagSqr += d*d
    d= bbox.zmax()-bbox.zmin()
    diagSqr += d*d
    diag= math.sqrt(diagSqr)
    fromPt= (atPt[0],atPt[1],atPt[2]+3*diag)
    mycam= starsplatter.Camera(fromPt,atPt,(0.0,1.0,0.0),
                               35.0, -1*diag, -5*diag)
    return mycam

class MyImageDialog(wxFrame):
    def __init__(self,parent,helpString,titleString):
        wxFrame.__init__(self,parent,wxID_ANY, titleString)
        self.bunchList= parent.bunchList
        self.bunchNameList= parent.bunchNameList
        self.camera= parent.camera
        self.renderer= parent.renderer
        self.cball= parent.cball
        box= wxBoxSizer(wxVERTICAL)
        self.helpString= helpString
        img= self.generateImage()
        if img:
            img.save("tmp.png","png")
            imageTmpFname= "tmp.png"
            tmp_image= wxImage(imageTmpFname,wxBITMAP_TYPE_PNG).ConvertToBitmap()
            os.unlink(imageTmpFname)
##             wxImg= wx.EmptyImage(img.xsize(),img.ysize())
##             tmp_image= wxImg.ConvertToBitmap()
            self.imageBitmap= wxStaticBitmap(self,-1,tmp_image)
            box.Add(self.imageBitmap,1,wxEXPAND,wxALL,10)
        buttonBox= wxBoxSizer(wxHORIZONTAL)
        mID= wxNewId()
        self.closeButton= wxButton(self,mID,"&Close")
        EVT_BUTTON(self, mID, self.OnClose)
        self.helpButton= wxButton(self,wxNewId(),"&Help")
        EVT_BUTTON(self, self.helpButton.GetId(), self.OnHelp)
        buttonBox.Add(self.closeButton,0,0)
        buttonBox.Add(self.helpButton,0,0)
        box.Add(buttonBox,0,wxALIGN_CENTER|wxALL,10)
        self.SetSizer(box)
        self.SetAutoLayout(True)
        box.SetSizeHints(self)
        self.Layout()
        self.Show(1)
    def OnClose(self,e):
        self.Close()
    def OnHelp(self,e):
        dlg= wxMessageDialog(self,self.helpString,"Info",\
                             wxOK|wxICON_INFORMATION)
        dlg.ShowModal()
        dlg.Destroy()
    def generateImage(self):
        myren= self.renderer
        xDim,yDim= imageDims
        myren.set_image_dims(xDim,yDim)
        myren.set_camera(self.camera)
        myren.set_transform(self.cball.getCumulativeViewTrans())
        myren.set_exposure_type( starsplatter.StarSplatter.ET_LOG_AUTO )
        myren.set_exposure_scale(0.5)
        myren.set_debug(debugFlag)

        print("The valid camera is %s"%self.camera)
        myren.clear_stars()
        for i in range(len(self.bunchList)):
            sb= self.bunchList[i]
            sbName= self.bunchNameList[i]
            #sys.stderr.write("#### bunch %s:\n"%sbName)
            #sb.dump(sys.stderr)
            myren.add_stars(sb)
        try:
            img= myren.render()
            black= starsplatter.rgbImage(img.xsize(),img.ysize())
            black.clear((0.0,0.0,0.0,1.0))
            img.add_under(black)
            #img.save("test_over.png","png")
            return img
        except:
            print("Render failed!")
            return None

class MyTextDialog(wxFrame):
    def __init__(self,parent,textString,helpString,titleString):
        wxFrame.__init__(self,parent,wxID_ANY, titleString)
        box= wxBoxSizer(wxVERTICAL)
        self.helpString= helpString
        self.text= wxTextCtrl(self, 1,
                              style=wxTE_MULTILINE|wxTE_READONLY|wxHSCROLL|\
                              wxTE_DONTWRAP,
                              size=(300,200))
        self.text.SetValue(textString)
        box.Add(self.text,1,wxEXPAND,wxALL,10)
        buttonBox= wxBoxSizer(wxHORIZONTAL)
        mID= wxNewId()
        self.closeButton= wxButton(self,mID,"&Close")
        EVT_BUTTON(self, mID, self.OnClose)
        self.helpButton= wxButton(self,wxNewId(),"&Help")
        EVT_BUTTON(self, self.helpButton.GetId(), self.OnHelp)
        buttonBox.Add(self.closeButton,0,0)
        buttonBox.Add(self.helpButton,0,0)
        box.Add(buttonBox,0,wxALIGN_CENTER|wxALL,10)
        self.SetSizer(box)
        self.SetAutoLayout(True)
        box.SetSizeHints(self)
        self.Layout()
        self.Show(1)
    def OnClose(self,e):
        self.Close()
    def OnHelp(self,e):
        dlg= wxMessageDialog(self,self.helpString,"Info",\
                             wxOK|wxICON_INFORMATION)
        dlg.ShowModal()
        dlg.Destroy()

class MyFloatTextCtrl(wxTextCtrl):
    def __init__(self, parent, mID, initialValue, callback=None, hookData=None,
                 size=(80,40)):
        wxTextCtrl.__init__(self,parent,mID,size=size,style=wxTE_PROCESS_ENTER)
        EVT_TEXT_ENTER(self,mID,self.textChangeCB)
        self.val= initialValue
        self.mainCallback= callback
        self.updateDisplayedVal()
    def updateDisplayedVal(self):
        self.SetValue("%g"%self.val)
    def textChangeCB(self,e):
        self.val= float(self.GetValue())
        if self.mainCallback != None:
            self.mainCallback(e)
    def setFloat(self,val):
        self.val= val
        self.updateDisplayedVal()
    def getFloat(self):
        return self.val

class ScaledSlider:
    def __init__(self, parent, initialValue, callback, titleString,
                 hookData=None):
        self.box= wxBoxSizer(wxHORIZONTAL)
        mID= wxNewId()
        self.box.Add(wxStaticText(parent,mID,titleString,size=(80,40)),
                      0,wxALIGN_CENTER,0)
        mID= wxNewId()
        self.scaleText= wxTextCtrl(parent,mID,size=(80,40),
                                   style=wxTE_PROCESS_ENTER)
        EVT_TEXT_ENTER(parent,mID,self.scaleTextCallback)
        self.box.Add(self.scaleText)
        mID= wxNewId()
        self.slider= wxSlider(parent,mID,0,10,99,size=(200,40),
                         name=titleString,style=wxSL_HORIZONTAL)
        EVT_COMMAND_SCROLL(parent,mID,self.sliderCallback)
        self.box.Add(self.slider,1,wxEXPAND|wxALL,0)
        self.val= initialValue
        self.mainCallback= callback
        self.hookData= hookData
        self.updateDisplayedVal()

    def updateDisplayedVal(self):
        if (self.val != 0.0):
            self.exponentPart= math.pow(10.0,math.floor(math.log10(self.val)))
        else:
            self.exponentPart= 1.0
        self.abscissa= self.val/self.exponentPart
        self.scaleText.SetValue("%g"%self.val)
        self.slider.SetValue(10*self.abscissa)        
    def getBox(self):
        return self.box
    def getValue(self):
        return self.val
    def setValue(self,val):
        self.val= val
        self.updateDisplayedVal()
#    def getDisplayedVal(self):
#        return float(self.scaleText.GetValue())\
#               *((1.0/10.0)*self.slider.GetValue())
    def sliderCallback(self,e):
        self.val= self.exponentPart*((1.0/10.0)*self.slider.GetValue())
##        print "exponentPart %g, slider %g -> %g"%\
##              (self.exponentPart,((1.0/10.0)*self.slider.GetValue()),
##               self.val)
        self.updateDisplayedVal()
        self.mainCallback(self.val,self.hookData)
    def scaleTextCallback(self,e):
        self.val= float(self.scaleText.GetValue())
        self.updateDisplayedVal()
        self.mainCallback(self.val,self.hookData)

def makeSlider(parent,label,callback,initialValue=0.0):
    littleBox= wxBoxSizer(wxHORIZONTAL)
    mID= wxNewId()
    littleBox.Add(wxStaticText(parent,mID,label,size=(80,40)),
                  0,wxALIGN_CENTER,0)
    mID= wxNewId()
    slider= wxSlider(parent,mID,0,0,100,size=(200,40),
                     name=label,style=wxSL_HORIZONTAL)
    slider.SetValue(100*initialValue)
    EVT_COMMAND_SCROLL(parent,mID,callback)
    littleBox.Add(slider,1,wxEXPAND|wxALL,0)
    return (littleBox, slider)

class MyBunchDialog(wxFrame):
    def __init__(self,parent,bunch,dsName,bunchName,titleString):
        wxFrame.__init__(self,parent,wxID_ANY, titleString)
        self.bunch= bunch
        self.bunchName= bunchName
        self.unscaledBunchClr= list(bunch.bunch_color())
        if (self.unscaledBunchClr[3]!=0.0):
            self.unscaledBunchClr[0] /= self.unscaledBunchClr[3]
            self.unscaledBunchClr[1] /= self.unscaledBunchClr[3]
            self.unscaledBunchClr[2] /= self.unscaledBunchClr[3]
        box= wxBoxSizer(wxVERTICAL)
        self.helpString= "This dialog allows you to edit the attributes for "\
                         +" the %s bunch from dataset %s"%\
                         (bunchName,dsName)

        (littleBox, self.opacitySlider)= makeSlider(self,"opacity",
                                                    self.OnOpacitySliderMotion,
                                                    bunch.bunch_color()[3])
        box.Add(littleBox,0,wxEXPAND|wxALL,10)

        self.densitySlider= ScaledSlider(self,bunch.density(),
                                         self.onDensitySliderChange,"Density")
        box.Add(self.densitySlider.getBox(),0,wxEXPAND|wxALL,10)

        self.scaleLengthSlider= ScaledSlider(self,bunch.scale_length(),
                                             self.onScaleLengthSliderChange,
                                             "Scale Length")
        box.Add(self.scaleLengthSlider.getBox(),0,wxEXPAND|wxALL,10)

        self.text= None # Avoid a creation order problem
        
        mID= wxNewId()
        self.colorModeChoicebook= wxChoicebook(self,mID)
        EVT_CHOICEBOOK_PAGE_CHANGED(self.colorModeChoicebook,mID,
                                    self.onColorMethodChoiceChange)
        self.colorModeList= [("Constant",self.prepConstColor,
                              self.setConstColor),
                             ("CMap1D",self.prepCmap1DColor,
                              self.setCmap1DColor),
                             ("CMap2D",self.prepCmap2DColor,
                              self.setCmap2DColor)]
        for (pageName,createCtls,setMode) in self.colorModeList:
            win= wxPanel(self.colorModeChoicebook)
            createCtls(win)
            self.colorModeChoicebook.AddPage(win,pageName)
        box.Add(self.colorModeChoicebook)
        
        self.text= wxTextCtrl(self, 1,
                              style=wxTE_MULTILINE|wxTE_READONLY|wxHSCROLL|\
                              wxTE_DONTWRAP,
                              size=(200,200))
        self.updateTextDescription()
        box.Add(self.text,1,wxEXPAND,wxALL,10)
        buttonBox= wxBoxSizer(wxHORIZONTAL)
        mID= wxNewId()
        self.closeButton= wxButton(self,mID,"&Close")
        EVT_BUTTON(self, mID, self.OnClose)
        self.helpButton= wxButton(self,wxNewId(),"&Help")
        EVT_BUTTON(self, self.helpButton.GetId(), self.OnHelp)
        buttonBox.Add(self.closeButton,0,0)
        buttonBox.Add(self.helpButton,0,0)
        box.Add(buttonBox,0,wxALIGN_CENTER|wxALL,10)
        self.SetSizer(box)
        self.SetAutoLayout(True)
        box.SetSizeHints(self)
        self.Layout()
        self.Show(1)
    def OnClose(self,e):
        self.Close()
    def OnHelp(self,e):
        dlg= wxMessageDialog(self,self.helpString,"Info",\
                             wxOK|wxICON_INFORMATION)
        dlg.ShowModal()
        dlg.Destroy()
    def setBunchClrFromUnscaled(self):
        scaledClr= ( self.unscaledBunchClr[0]*self.unscaledBunchClr[3],
                     self.unscaledBunchClr[1]*self.unscaledBunchClr[3],
                     self.unscaledBunchClr[2]*self.unscaledBunchClr[3],
                     self.unscaledBunchClr[3] )
        self.bunch.set_bunch_color(scaledClr)
        self.updateTextDescription()
    def OnOpacitySliderMotion(self,e):
        range= float(self.opacitySlider.GetMax()-self.opacitySlider.GetMin())
        newOpacity= float(self.opacitySlider.GetValue())/range
        self.unscaledBunchClr[3]= newOpacity
        self.setBunchClrFromUnscaled()
    def onDensitySliderChange(self,val,hookData):
        newDensity= val
        self.bunch.set_density(val)
        self.updateTextDescription()
    def onScaleLengthSliderChange(self,val,hookData):
        self.bunch.set_scale_length(val)
        self.updateTextDescription()
    def onColorMethodChoiceChange(self,e):
        (modeName,createCtls,setMode)= self.colorModeList[e.GetSelection()]
        setMode()
        self.updateTextDescription()
    def updateTextDescription(self):
        if self.text==None: return
        bunch= self.bunch
        textString= "Stars: %d\n"%bunch.nstars()
        textString += "%d properties: \n"%bunch.nprops()
        for i in range(bunch.nprops()):
            textString += "   %d: %s\n"%(i,bunch.propName(i))
        textString += "bunch color RGBA= %s\n"%str(bunch.bunch_color())
        textString += "bunch density: %g\n"%bunch.density()
        textString += "bunch scale length: %g\n"%bunch.scale_length()
        cmapModeID= bunch.attr(starsplatter.StarBunch.COLOR_ALG)
        if cmapModeID==starsplatter.StarBunch.CM_CONSTANT:
            cmapModeString= "constant"
        elif cmapModeID==starsplatter.StarBunch.CM_COLORMAP_1D:
            cmapModeString= "1D map on attribute %d"%\
                            bunch.attr(starsplatter.StarBunch.COLOR_PROP1)
        elif cmapModeID==starsplatter.StarBunch.CM_COLORMAP_2D:
            cmapModeString= "2D map on attributes %d, %d"%\
                            (bunch.attr(starsplatter.StarBunch.COLOR_PROP1),
                            bunch.attr(starsplatter.StarBunch.COLOR_PROP2))
        textString += "color map mode: %s\n"%cmapModeString
        self.text.SetValue(textString)
 
    def OnEditColor(self,e):
        clrList= self.unscaledBunchClr
        print("old color : %s"%str(self.unscaledBunchClr))
        oldClr= wxColour(math.floor((255*(clrList[0]/clrList[3]))+0.5),
                         math.floor((255*(clrList[1]/clrList[3]))+0.5),
                         math.floor((255*(clrList[2]/clrList[3]))+0.5))
        clrData= wxColourData()
        clrData.SetColour(oldClr)
        dlg= wxColourDialog(self,clrData)
        dlg.ShowModal()
        newClrData= dlg.GetColourData()
        newClr= newClrData.GetColour()
        self.unscaledBunchClr[0]= newClr.Red()/255.0
        self.unscaledBunchClr[1]= newClr.Green()/255.0
        self.unscaledBunchClr[2]= newClr.Blue()/255.0
        self.setBunchClrFromUnscaled()
        dlg.Destroy()

    def prepConstColor(self,parent):
        box= wxBoxSizer(wxVERTICAL)
        mID= wxNewId()
        pickColorButton= wxButton(parent, mID, "Edit Color")
        EVT_BUTTON(parent, mID, self.OnEditColor)
        box.Add(pickColorButton,0,0)

    def setConstColor(self):
            self.bunch.set_attr(starsplatter.StarBunch.COLOR_ALG,
                                starsplatter.StarBunch.CM_CONSTANT)

    def prepCmap1DColor(self,panel):
        propList= []
        bunch= self.bunch
        for i in range(bunch.nprops()):
            propList.append(bunch.propName(i))
        box= wxBoxSizer(wxVERTICAL)

        hbox= wxBoxSizer(wxHORIZONTAL)
        mID= wxNewId()
        lbl= wxStaticText(panel,mID,"Map property:")
        hbox.Add(lbl, 0, wxALL, 10)
        mID= wxNewId()
        propChoice= wxChoice(panel,mID,choices=propList)
        propChoice.SetSelection(0)
        EVT_CHOICE(propChoice,mID,self.updateCmap1DColorSettings)
        hbox.Add(propChoice, 1, wxALL, 10)
        box.Add(hbox)

        hbox= wxBoxSizer(wxHORIZONTAL)
        mID= wxNewId()
        lbl= wxStaticText(panel,mID,"Color Map")
        hbox.Add(lbl, 0, wxALL, 10)
        mID= wxNewId()
        mapChoice= wxChoice(panel,mID,choices=["RedGreen","BlueYellow"])
        mapChoice.SetSelection(0)
        EVT_CHOICE(mapChoice,mID,self.updateCmap1DColorSettings)
        hbox.Add(mapChoice, 1, wxALL, 10)
        box.Add(hbox)

        hbox= wxBoxSizer(wxHORIZONTAL)
        hbox.Add(wxStaticText(panel,-1,"Range:"),0,wxALL,10)
        mID= wxNewId()
        minText= MyFloatTextCtrl(panel,mID,-50.0,
                                 self.updateCmap1DColorSettings)
        hbox.Add(minText,0,wxALL,10)
        mID= wxNewId()
        maxText= MyFloatTextCtrl(panel,mID,50.0,
                                 self.updateCmap1DColorSettings)
        hbox.Add(maxText,0,wxALL,10)    
        box.Add(hbox)
        panel.SetSizer(box)
        self.cmap1DInfo= (minText, maxText, mapChoice, propChoice)
        self.updateCmap1DColorSettings(None)

    def setCmap1DColor(self):
        self.bunch.set_attr(starsplatter.StarBunch.COLOR_ALG,
                            starsplatter.StarBunch.CM_COLORMAP_1D)        
        self.updateCmap1DColorSettings(None)
        self.updateTextDescription()
        self.bunch.dump(sys.stderr)

    def updateCmap1DColorSettings(self,e):
        (minText, maxText, mapChoice, propChoice)= self.cmap1DInfo
        cmapName= mapChoice.GetString(mapChoice.GetSelection())
        if cmapName=="RedGreen":
            cmap= [(1.0,0.0,0.0,1.0),
                   (0.0,1.0,0.0,1.0)]
        elif cmapName=="BlueYellow":
            cmap= [(0.0,0.0,1.0,1.0),
                   (1.0,1.0,0.0,1.0)]
        else:
            print("Internal error: no know cmap for %s"%cmapName)
            cmap= [(1.0,0.0,0.0,1.0),
                   (0.0,1.0,0.0,1.0)]
            
        self.bunch.set_colormap1D(cmap,
                                  minText.getFloat(), maxText.getFloat())
        self.bunch.set_attr(starsplatter.StarBunch.COLOR_PROP1,
                            propChoice.GetSelection());
        self.updateTextDescription()

    def prepCmap2DColor(self,panel):
        propList= []
        bunch= self.bunch
        for i in range(bunch.nprops()):
            propList.append(bunch.propName(i))
        box= wxBoxSizer(wxVERTICAL)
        hbox= wxBoxSizer(wxHORIZONTAL)
        mID= wxNewId()
        lbl= wxStaticText(panel,mID,"Map properties:")
        hbox.Add(lbl, 0, wxALL, 10)
        mID= wxNewId()
        choice= wxChoice(panel,mID,choices=propList)
        hbox.Add(choice, 1, wxALL, 10)
        mID= wxNewId()
        choice= wxChoice(panel,mID,choices=propList)
        hbox.Add(choice, 1, wxALL, 10)
        box.Add(hbox)
        hbox= wxBoxSizer(wxHORIZONTAL)
        mID= wxNewId()
        lbl= wxStaticText(panel,mID,"Color Map")
        hbox.Add(lbl, 0, wxALL, 10)
        mID= wxNewId()
        choice= wxChoice(panel,mID,choices=["RedGreen/BlackYellow"])
        hbox.Add(choice, 1, wxALL, 10)
        box.Add(hbox)

        hbox= wxBoxSizer(wxHORIZONTAL)
        hbox.Add(wxStaticText(panel,-1,"Ranges:"),0,wxALL,10)
        mID= wxNewId()
        minText1= wxTextCtrl(panel, mID, size=(80,40),style=wxTE_PROCESS_ENTER)
        hbox.Add(minText1,0,wxALL,2)
        mID= wxNewId()
        maxText1= wxTextCtrl(panel, mID, size=(80,40),style=wxTE_PROCESS_ENTER)
        hbox.Add(maxText1,0,wxALL,2)    
        mID= wxNewId()
        minText2= wxTextCtrl(panel, mID, size=(80,40),style=wxTE_PROCESS_ENTER)
        hbox.Add(minText2,0,wxALL,2)
        mID= wxNewId()
        maxText2= wxTextCtrl(panel, mID, size=(80,40),style=wxTE_PROCESS_ENTER)
        hbox.Add(maxText2,0,wxALL,2)    
        box.Add(hbox)

        panel.SetSizer(box)

        self.bunch.set_attr(starsplatter.StarBunch.COLOR_PROP1,1);
        self.bunch.set_attr(starsplatter.StarBunch.COLOR_PROP2,2);
        self.bunch.set_colormap2D([[(1.0,0.0,0.0,1.0),(0.0,0.0,1.0,1.0)],
                                   [(1.0,0.3,0.0,1.0),(0.0,0.3,1.0,1.0)]],
                                  -100.0, 100.0, -100.0, 100.0)

    def setCmap2DColor(self):
        self.bunch.set_attr(starsplatter.StarBunch.COLOR_ALG,
                            starsplatter.StarBunch.CM_COLORMAP_2D)        

class MainWindow(wxFrame):
    def __init__(self,parent,id,title):
        wxFrame.__init__(self,parent,wxID_ANY, title, \
                         style=wxDEFAULT_FRAME_STYLE|\
                         wxNO_FULL_REPAINT_ON_RESIZE,
                         size=(500,450))

        self.historyFrame= None
        self.bunchList= []
        self.bunchNameList= []
        self.bunchDict= {}
        self.dsNameList= []
        self.bbox= None
        cam= self.camera= createDefaultCamera()
        self.cball= starsplatter.CBall(None,None)
        self.matchCBallToCam()
        self.renderer= starsplatter.StarSplatter()
        self.bunchSubMenu= None

        self.CreateStatusBar() # A Statusbar in the bottom of the window
        # Setting up the menu.
        filemenu= wxMenu()
        mID= wxNewId()
        filemenu.Append(mID,"O&pen..."," Open a file")
        EVT_MENU(self, mID, self.OnOpen)
        mID= wxNewId()
        filemenu.Append(mID,"A&dd..."," Open a file")
        EVT_MENU(self, mID, self.OnAdd)
        mID= wxNewId()
        filemenu.Append(mID,"Test Geom"," Create test geometry")
        EVT_MENU(self, mID, self.OnCreateTestGeom)
        mID= wxNewId()
        filemenu.Append(mID,"C&lose"," Close this dataset")
        EVT_MENU(self, mID, self.OnClose)
        self.filemenu= filemenu
        self.viewmenu= wxMenu()
        mID= wxNewId()
        self.viewmenu.Append(mID,"I&mage"," View file as an image")
        EVT_MENU(self, mID, self.OnImage)
        for item in self.viewmenu.GetMenuItems():
            item.Enable(False)
        # Creating the menubar.
        menuBar = wxMenuBar()
        menuBar.Append(filemenu,"&File") # Adding the "filemenu" to the MenuBar
        menuBar.Append(self.viewmenu,"&View")
        self.SetMenuBar(menuBar)  # Adding the MenuBar to the Frame content.
        self.SetStatusText("File name not yet set")

        # Set up the main panel
        panel= wxPanel(self,-1)
        box= wxBoxSizer(wxHORIZONTAL)
        innerBox= wxBoxSizer(wxVERTICAL)
        # Set up the crystal ball area
        mID= wxNewId()
        self.cballCanvas= wxWindow(panel, mID, size=(200,200),
                              style= wxRAISED_BORDER)
        EVT_LEFT_DOWN(self.cballCanvas,self.OnLeftMouseDown)
        EVT_LEFT_UP(self.cballCanvas,self.OnLeftMouseUp)
        EVT_MIDDLE_DOWN(self.cballCanvas,self.OnMiddleMouseDown)
        EVT_MIDDLE_UP(self.cballCanvas,self.OnMiddleMouseUp)
        innerBox.Add(self.cballCanvas, 0, wxSHAPED, 10)
##         self.dollySlider= wxSlider(panel,wxNewId(),50,0,100,size=(200,40),
##                               name="Dolly",style=wxSL_HORIZONTAL)
##         EVT_COMMAND_SCROLL(panel, self.dollySlider.GetId(), self.OnDolly)
##         EVT_COMMAND_SCROLL_CHANGED(panel, self.dollySlider.GetId(),
##                                         self.OnDollyRelease)
        self.dollySlider= ScaledSlider(panel,self.camera.view_dist(),
                                       self.OnDollySliderChange,"Dolly")
        innerBox.Add(self.dollySlider.getBox(),0,wxEXPAND|wxALL,10)
        self.hitherSlider= ScaledSlider(panel,-self.camera.hither_dist(),
                                        self.OnHitherSliderChange,"Hither")
        innerBox.Add(self.hitherSlider.getBox(),0,wxEXPAND|wxALL,10)
        self.yonSlider= ScaledSlider(panel,-self.camera.yon_dist(),
                                     self.OnYonSliderChange,"Yon")
        innerBox.Add(self.yonSlider.getBox(),0,wxEXPAND|wxALL,10)
        box.Add(innerBox)
        vbox= wxBoxSizer(wxVERTICAL)
        mID= wxNewId()
        button= wxButton(panel,mID,"&Reset Trackball")
        EVT_BUTTON(button,mID,self.OnTrackballReset)
        vbox.Add(button,0,0,10)
        mID= wxNewId()
        button= wxButton(panel,mID,"Reset &Camera")
        EVT_BUTTON(button,mID,self.OnCameraReset)
        vbox.Add(button,0,0,10)
        mID= wxNewId()
        button= wxButton(panel,mID,"&Crop To Slice")
        EVT_BUTTON(button,mID,self.OnCrop)
        vbox.Add(button,0,0,10)
        mID= wxNewId()
        button= wxButton(panel,mID,"&Print state")
        # EVT_BUTTON(button,mID,self.OnTrackballReset)
        vbox.Add(button,0,0,10)
        box.Add(vbox)
        panel.SetSizer(box)

        self.Show(1)

    def updateCamFromControls(self):
        (atX, atY, atZ, atW)= self.camera.atpt()
        (fmX, fmY, fmZ, fmW)= self.camera.frompt()
        atX /= atW
        atY /= atW
        atZ /= atW
        atW= 1.0
        dX= atX-(fmX/fmW)
        dY= atY-(fmY/fmW)
        dZ= atZ-(fmZ/fmW)
        sep= math.sqrt( (dX*dX) + (dY*dY) + (dZ*dZ) )
        newSep= self.dollySlider.getValue()
        newDX= dX*(newSep/sep)
        newDY= dY*(newSep/sep)
        newDZ= dZ*(newSep/sep)
        self.camera= starsplatter.Camera((atX+newDX,atY+newDY,atZ+newDZ,atW),
                                         (atX,atY,atZ,atW),
                                         self.camera.updir(),
                                         self.camera.fov(),
                                         -self.hitherSlider.getValue(),
                                         -self.yonSlider.getValue())
            
    def updateControlsFromCam(self):
        self.dollySlider.setValue(self.camera.view_dist())
        self.hitherSlider.setValue(-self.camera.hither_dist())
        self.yonSlider.setValue(-self.camera.yon_dist())

    def matchCBallToCam(self):
        (atX, atY, atZ, atW)= self.camera.atpt()
        (fmX, fmY, fmZ, fmW)= self.camera.frompt()
        dX= (atX/atW)-(fmX/fmW)
        dY= (atY/atW)-(fmY/fmW)
        dZ= (atZ/atW)-(fmZ/fmW)
        sep= math.sqrt( (dX*dX) + (dY*dY) + (dZ*dZ) )
        scale= 2.0*sep*math.sin(0.5*(math.pi/180.0)*self.camera.fov())
        self.cball.set_scale(scale)

    def OnTrackballReset(self,e):
        self.cball.reset()

    def OnCameraReset(self,e):
        self.camera= createBBoxCamera(self.bbox)
        self.cball.reset() # because otherwise it gets out of sync w/ cam
        self.updateControlsFromCam()

    def OnCrop(self,e):
        sliceThickness= 0.01*(self.bbox.zmax()-self.bbox.zmin())
        ctr= self.bbox.center()
        print("Slice thickness is %g, center %s"%(sliceThickness,ctr))
        highpt= (ctr[0],ctr[1],ctr[2]+(0.5*sliceThickness*ctr[3]),ctr[3])
        lowpt= (ctr[0],ctr[1],ctr[2]-(0.5*sliceThickness*ctr[3]),ctr[3])
        for key in list(self.bunchDict.keys()):
            (name,sb,ds)= self.bunchDict[key]
            print("%s from %s starts with %d stars"%(name,ds,sb.nstars()))
            sb.crop(highpt,(0.0,0.0,-1.0))
            sb.crop(lowpt,(0.0,0.0,1.0))
            print("%s now has %d stars"%(name,sb.nstars()))

    def OnDollySliderChange(self,val,hookData):
        self.updateCamFromControls()

    def OnHitherSliderChange(self,val,hookData):
        self.updateCamFromControls()

    def OnYonSliderChange(self,val,hookData):
        self.updateCamFromControls()

    def OnLeftMouseDown(self,e):
        xSz, ySz= self.cballCanvas.GetSize()
        self.dragStart= (e.m_x, ySz-(e.m_y+1), xSz, ySz) 

    def OnLeftMouseUp(self,e):
        oldX, oldY, oldXSz, oldYSz= self.dragStart
        xSz, ySz= self.cballCanvas.GetSize()
        dragEnd= (e.m_x, ySz-(e.m_y+1), xSz, ySz)
        self.cball.roll(self.dragStart, dragEnd)
        self.dragStart= dragEnd

    def OnMiddleMouseDown(self,e):
        xSz, ySz= self.cballCanvas.GetSize()
        self.dragStart= (e.m_x, ySz-(e.m_y-1), xSz, ySz) 

    def OnMiddleMouseUp(self,e):
        oldX, oldY, oldXSz, oldYSz= self.dragStart
        xSz, ySz= self.cballCanvas.GetSize()
        dragEnd= (e.m_x, ySz-(e.m_y-1), xSz, ySz)
        self.cball.slide(self.dragStart, dragEnd)
        self.dragStart= dragEnd

    def OnClose(self,e):
        self.Close(true)  # Close the frame.

##     def implementDolly(self):
##         max= float(self.dollySlider.GetMax())
##         min= float(self.dollySlider.GetMin())
##         mid= 0.5*(max+min)
##         val= (self.dollySlider.GetValue()+mid)/(max-min)
##         (atX, atY, atZ, atW)= self.camera.atpt()
##         (fmX, fmY, fmZ, fmW)= self.camera.frompt()
##         dX= (atX/atW)-(fmX/fmW)
##         dY= (atY/atW)-(fmY/fmW)
##         dZ= (atZ/atW)-(fmZ/fmW)
##         dX *= val
##         dY *= val
##         dZ *= val
##         newFmX= (atX/atW)-dX
##         newFmY= (atX/atW)-dY
##         newFmZ= (atX/atW)-dZ
##         newCam= starsplatter.Camera( (newFmX, newFmY, newFmZ, 1.0),
##                                      self.camera.atpt(), self.camera.updir(),
##                                      self.camera.fov(),
##                                      self.camera.hither_dist(),
##                                      self.camera.yon_dist(),
##                                      self.camera.parallel_proj() )
##         print "Old camera: %s"%self.camera
##         print "New camera: %s"%newCam
##         self.camera= newCam
##         print val
##         self.dollySlider.SetValue(mid)

    def addBunchList(self,newBunchList,newBunchNameList):

        if not self.bunchSubMenu:
            self.bunchSubMenu= wxMenu()
            self.viewmenu.AppendMenu(wxNewId()," Bunch tags",
                                     self.bunchSubMenu)

        for i in range(len(newBunchList)):
            thisBunch= newBunchList[i]
            if debugFlag:
                thisBunch.set_attr(starsplatter.StarBunch.DEBUG_LEVEL,1)
            thisName= newBunchNameList[i]
            if thisBunch.nstars() > 0:
                if self.bbox:
                    self.bbox.union_with(thisBunch.boundBox())
                else: self.bbox= thisBunch.boundBox()
                mID= wxNewId()
                self.bunchSubMenu.Append(mID,str(thisName),\
                                         " %s attributes"%str(thisName))
                self.bunchDict[mID]= (thisName, thisBunch, self.dsNameList[-1])
                EVT_MENU(self,mID,self.OnBunchTag)
                self.bunchList.append(thisBunch)
                self.bunchNameList.append(thisName)
        
        print("Collected bunches have joint bbox %s"%self.bbox)
        self.camera= createBBoxCamera(self.bbox)
##         self.camera= starsplatter.Camera((0.0,0.0,30.0),
##                                               (0.0,0.0,0.0),
##                                               (0.0,1.0,0.0),
##                                               35.0,-20.0,-40.0)
        self.matchCBallToCam()
        self.updateControlsFromCam()

        for item in self.viewmenu.GetMenuItems():
            item.Enable(True)
        self.SetStatusText("This is my control widget!")
        if len(self.dsNameList)==1:
            self.SetTitle(self.dsNameList[0])
        elif len(self.dsNameList)>1:
            self.SetTitle("%s..."%self.dsNameList[0])
            

    def readBunchesFromFile(self,t,typeString="tipsy"):
        fileIsReadable= 1
        try:
            if not(os.access(t,os.F_OK)) :
                self.describeError("No such file %s!"%t)
            else:
                self.dsNameList.append(t)
        except Exception as e:
            fileIsReadable= 0
            mainWindow.describeError("Cannot open file: %s"%e)

        if not fileIsReadable: return # filename failed reality check
        if typeString=="dubinski":
            disk1= starsplatter.StarBunch()
            disk2= starsplatter.StarBunch()
            bulge1= starsplatter.StarBunch()
            bulge2= starsplatter.StarBunch()
            infile= open(t,"r")
            bunchList= [disk1,disk2,bulge1,bulge2]
            bunchNameList= ["disk1","disk2","bulge1","bulge2"]
            outList= starsplatter.load_dubinski(infile,bunchList)
            infile.close()
        elif typeString=="gadget":
            gas= starsplatter.StarBunch()
            halo= starsplatter.StarBunch()
            disk= starsplatter.StarBunch()
            bulge= starsplatter.StarBunch()
            stars= starsplatter.StarBunch()
            bndry= starsplatter.StarBunch()
            infile= open(t,"r")
            bunchList= [gas,halo,disk,bulge,stars,bndry]
            bunchNameList= ["gas","halo","disk","bulge","stars","bndry"]
            outList= starsplatter.load_gadget(infile,bunchList)

            # Drop the boundary points
            bunchList= bunchList[0:-2]
            bunchNameList= bunchNameList[0:-2]
            
            infile.close()
        elif typeString=="tipsy":
            gas= starsplatter.StarBunch()
            halo= starsplatter.StarBunch()
            stars= starsplatter.StarBunch()
            infile= open(t,"r")
            bunchList= [gas,halo,stars]
            bunchNameList= ["gas","halo","stars"]
            outList= starsplatter.load_tipsy_box_ascii(infile,gas,stars,halo)
        else:
            sys.exit("Unrecognized file type %s!"%typeString)

        return (bunchList, bunchNameList)

    def addFile(self,t,typeString="gadget"):
        (bunchList, bunchNameList)= self.readBunchesFromFile(t,typeString)
        for thisBunch in bunchList:
            setDefaultRenderingAttributes(thisBunch)
        self.addBunchList(bunchList, bunchNameList)

    def setFile(self,t,typeString):
        print("typestr is <%s>"%typeString)
        (bunchList, bunchNameList)= self.readBunchesFromFile(t,typeString)
        for thisBunch in bunchList:
            setDefaultRenderingAttributes(thisBunch)
        self.addBunchList(bunchList,bunchNameList)

    def OnOpen(self,e):
        fTypeList= ["gadget","tipsy","dubinski"]
        fWildcardString= "Gadget|*|TIPSY (*.tipsy)|*.tipsy|Dubinski (*.dubinski)|*.dubinski"
        dlg= wxFileDialog(self,wildcard=fWildcardString)
        dlg.ShowModal()
        fname= dlg.GetPath()
        fTypeString= fTypeList[dlg.GetFilterIndex()]
        if len(self.dsNameList) == 0:
            self.setFile(fname,fTypeString)
        else:
            newWindow= MainWindow(None, -1, "StarSplatter!")
            newWindow.setFile(fname,fTypeString)
        dlg.Destroy()

    def OnAdd(self,e):
        dlg= wxFileDialog(self,wildcard="*")
        dlg.ShowModal()
        fname= dlg.GetPath()
        if len(self.dsNameList) == 0:
            self.setFile(fname)
        else:
            self.addFile(fname)
        dlg.Destroy()

    def OnCreateTestGeom(self,e):
        newWindow= MainWindow(None, -1, "Test Geometry!")
        newWindow.dsNameList= ("Synthetic test geometry")

        group1= starsplatter.StarBunch()
        group1.set_nstars(4)
        group1.set_coords(0,(0.0,0.0,0.0))
        group1.set_coords(1,(5.0,0.0,0.0))
        group1.set_coords(2,(0.0,5.0,0.0))
        group1.set_coords(3,(0.0,0.0,5.0))
        group1.set_bunch_color((1.0,1.0,1.0,1.0))

        group2= starsplatter.StarBunch()
        group2.set_nstars(1)
        group2.set_coords(0,(-5.0,0.0,0.0))
        group2.set_bunch_color((1.0,0.0,0.0,1.0))
        
        group3= starsplatter.StarBunch()
        group3.set_nstars(1)
        group3.set_coords(0,(0.0,-5.0,0.0))
        group3.set_bunch_color((0.0,1.0,0.0,1.0))

        group4= starsplatter.StarBunch()
        group4.set_nstars(1)
        group4.set_coords(0,(0.0,0.0,-5.0))
        group4.set_bunch_color((0.0,0.0,1.0,1.0))

        bunchList= [group1, group2, group3, group4]
        for sb in bunchList:
            sb.set_density(0.03)
            sb.set_scale_length(0.1)
        newWindow.setBunchList(bunchList,
                               ["others","minX","minY","minZ"])
        newWindow.SetStatusText("Test geometry constructed!")
        newWindow.SetTitle("Cam Control Test Points")

    def OnImage(self,e):
##         self.implementDolly();
        imgTitle= "I have no starbunches!"
        if len(self.dsNameList)==1:
            imgTitle= self.dsNameList[0]
        elif len(self.dsNameList)>1:
            imgTitle= "%s..."%self.dsNameList[0]
        MyImageDialog(self,
                      "It's just an image.\nDeal with it.",
                      imgTitle)
        
    def OnBunchTag(self,e):
        (bunchName, bunch, ds)= self.bunchDict[e.GetId()]
        dlg= MyBunchDialog(self,bunch,ds,bunchName,
                          "%s from %s"%(bunchName,
                                        os.path.basename(ds)))

    def describeError(self,t):
        dlg= wxMessageDialog(self,t, "Something Bad!",wxICON_ERROR|wxOK)
        dlg.ShowModal()
        dlg.Destroy()

    def describeSelf(self):
        print("%s: usage: to be determined!"%sys.argv[0])
        
##############################
#
# Main
#
##############################

# Check for "-help"
if len(sys.argv)>1:
    if sys.argv[1] == "-help":
        self.describeSelf();
	sys.exit();

try:
    (opts,pargs) = getopt.getopt(sys.argv[1:],"vd",[])
except:
    print("%s: Invalid command line parameter" % sys.argv[0])
    describeSelf();
    time.sleep(5)
    sys.exit()

for a,b in opts:
    if a=="-v":
        verboseFlag= 1
    if a=="-d":
        debugFlag= 1

app = wxPySimpleApp()
mainWindow= MainWindow(None, -1, "StarSplatter!")

if len(pargs)>0:
    mainWindow.setFile(os.path.abspath(pargs[0]))

app.MainLoop()
