﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using GbReaper.Classes;
using GbReaper.Forms;

namespace GbReaper.Controls {
    public partial class UcMapEditor : UserControl {
        public const int TILE_SIZE = Tile.HEIGHT_PX * 3;

        protected bool mShowGrid = true;

        protected Map mCurrentMap = null;
        protected Tile mCurrentTile = null;

        
        public Map CurrentMap { 
            get { return this.mCurrentMap; } 
            set {
                if (this.mCurrentMap != null) {
                    this.mCurrentMap.MapChanged -= new EventHandler(CurrentMap_MapChanged);
                }

                this.mCurrentMap = value;

                this.mCurrentMap.MapChanged -= new EventHandler(CurrentMap_MapChanged);
                this.mCurrentMap.MapChanged += new EventHandler(CurrentMap_MapChanged);

                this.Invalidate(); 
            } 
        }
        public Tile CurrentTile {
            get { return this.mCurrentTile; }
            set { this.mCurrentTile = value; }
        }
        protected Rectangle GridBorders {
            get {
                return new Rectangle(
                ((panMap.Width - TILE_SIZE * this.mCurrentMap.Width) / 2),
                ((panMap.Height - TILE_SIZE * this.mCurrentMap.Height) / 2),
                TILE_SIZE * this.mCurrentMap.Width,
                TILE_SIZE * this.mCurrentMap.Height);
            }
        }


        public event EventHandler NewMap;


        public UcMapEditor() {
            InitializeComponent();

            panMap.Paint += new PaintEventHandler(panMap_Paint);
            panMap.MouseDown += new MouseEventHandler(panMap_MouseDown);
            panMap.MouseMove += new MouseEventHandler(panMap_MouseMove);
        }

        void panMap_MouseMove(object sender, MouseEventArgs e) {
            MousePaintCell(e);
                        
        }

        private void MousePaintCell(MouseEventArgs e) {
            if (this.mCurrentTile == null || this.mCurrentMap == null)
                return;

            if (e.Button == System.Windows.Forms.MouseButtons.Left || e.Button == System.Windows.Forms.MouseButtons.Right) {
                Rectangle vBorders = GridBorders;
                if (!vBorders.Contains(e.Location))
                    return;

                Point vP = new Point(
                    (e.X - vBorders.X) / TILE_SIZE,
                    (e.Y - vBorders.Y) / TILE_SIZE
                    );

                if (e.Button == System.Windows.Forms.MouseButtons.Left) {
                    if (this.mCurrentMap[vP.X, vP.Y] != null && this.mCurrentMap[vP.X, vP.Y].Equals(this.mCurrentTile)) {
                        //ignore, already set
                    }
                    else {
                        //set and repaint
                        this.mCurrentMap.SetTile(this.mCurrentTile, vP.X, vP.Y);
                        this.panMap.Invalidate();
                    }
                }
                else { 
                    //right click clears
                    this.mCurrentMap.ClearTileAt(vP.X, vP.Y);
                    this.panMap.Invalidate();
                }
            }
        }

        void panMap_MouseDown(object sender, MouseEventArgs e) {
            MousePaintCell(e);
        }

        void panMap_Paint(object sender, PaintEventArgs e) {
            if (this.mCurrentMap == null) {
                e.Graphics.DrawString("no map selected", this.panMap.Font, Brushes.Red, e.ClipRectangle, StringFormat.GenericDefault);
                return;
            }

            //make the grid
            int vMaxX = e.ClipRectangle.Width / TILE_SIZE;
            int vMaxY = e.ClipRectangle.Height / TILE_SIZE;

            //Get the limits
            Rectangle vBorders = GridBorders;

            //draw grid
            if (mShowGrid) {
                DrawingLogic.DrawGrid(e, vBorders, Pens.LightGray, this.mCurrentMap.Width, this.mCurrentMap.Height);
            }

            //draw the external borders
            e.Graphics.DrawRectangle(Pens.Turquoise, vBorders);

            //if (mSelectedPoint != Point.Empty && vBorders.Contains(mSelectedPoint)) {
            //    Rectangle vSelected = new Rectangle(
            //        vBorders.X + TILE_SIZE * ((mSelectedPoint.X - vBorders.X) / TILE_SIZE) ,
            //        vBorders.Y + TILE_SIZE * ((mSelectedPoint.Y - vBorders.Y) / TILE_SIZE),
            //        TILE_SIZE,
            //        TILE_SIZE);
            //    e.Graphics.DrawRectangle(Pens.Red, vSelected);

            //}


            //Draw content at last
            DrawingLogic.SetGraphicsNoInterpol(e.Graphics);
            for (int x = 0; x < this.mCurrentMap.Width; x++) { 
                for (int y = 0; y < this.mCurrentMap.Height; y++) {
                    Tile vT = this.mCurrentMap[x, y];

                    if (vT != null) {
                        e.Graphics.DrawImage(vT.Image,
                            vBorders.X + x * TILE_SIZE,
                            vBorders.Y + y * TILE_SIZE,
                            TILE_SIZE,
                            TILE_SIZE
                            );
                    }
                } 
            }
        }

        private void btnNew_Click(object sender, EventArgs e) {
            //using (FrmNewMap vFrm = new FrmNewMap()) {
            //    if (DialogResult.OK == vFrm.ShowDialog(this)) {
            //        if (this.mCurrentMap != null) {
            //            this.mCurrentMap.MapChanged -= new EventHandler(CurrentMap_MapChanged);
            //        }

            //        this.mCurrentMap = new Map(vFrm.CreateWidth, vFrm.CreateHeight);
            //        this.mCurrentMap.MapChanged -= new EventHandler(CurrentMap_MapChanged);
            //        this.mCurrentMap.MapChanged += new EventHandler(CurrentMap_MapChanged);
            //        this.mCurrentMap.Name = vFrm.CreateName;
                    
            //        this.Invalidate();
            //        //this.Refresh();
            //    }
            //}

            OnNewMap();

        }

        protected void OnNewMap() {
            if (this.NewMap != null) {
                this.NewMap(this, EventArgs.Empty);
            }
        }
        
        void CurrentMap_MapChanged(object sender, EventArgs e) {
            this.Invalidate();
            //this.Refresh();
        }

        private void btnGrid_Click(object sender, EventArgs e) {
            mShowGrid = !mShowGrid;
            this.panMap.Invalidate();
        }

        private void btnTilizator_Click(object sender, EventArgs e) {
            using (FrmTilizator vFrm = new FrmTilizator()) {
                if (vFrm.ShowDialog(this) == DialogResult.OK) { 
                    //Start: create target bitmap with right size
                    Bitmap vTarget = DrawingLogic.CopyAndResize(vFrm.CreateBmp, Tile.WIDTH_PX * vFrm.CreateWidth, Tile.HEIGHT_PX * vFrm.CreateHeight);

                    //map colors
                    Palette vPal = Palette.DEFAULT_PALETTE;
                    DrawingLogic.MapBitmapColorsToPalette(vTarget, vPal);

                    //tilization
                    int vTileNewCount = 0, vTileReusedCount = 0;
                    for (int x = 0; x < vFrm.CreateWidth; x++) {
                        for (int y = 0; y < vFrm.CreateHeight; y++) {
                            Rectangle vTileRect = new Rectangle(x * Tile.WIDTH_PX, y * Tile.HEIGHT_PX, Tile.WIDTH_PX, Tile.HEIGHT_PX);
                            Bitmap vTileBmp = (Bitmap)vTarget.Clone(vTileRect, vTarget.PixelFormat);

                            //make new tile and add/get similar from library
                            Tile vT = new Tile(vTileBmp, vPal);
                            bool vTileAlreadyExisted = false;
                            vT = this.mCurrentMap.ParentProject.mLibraries[0].AddTileWithoutDuplicate(vT, out vTileAlreadyExisted);

                            if (!vTileAlreadyExisted) 
                                vTileNewCount++;
                            else 
                                vTileReusedCount++;

                            //apply the tile to the map
                            this.mCurrentMap.SetTile(vT, vFrm.CreateLeft + x, vFrm.CreateTop+ y);
                        }
                    }

                    //refresh
                    panMap.Invalidate();

                    ((FrmMain)this.FindForm()).SetStatus("Tilization: generated " + vTileNewCount + " tiles, reused "+vTileReusedCount+" tiles.");
                }
            }
        }



    }
}