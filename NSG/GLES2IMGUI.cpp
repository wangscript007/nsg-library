/*
-------------------------------------------------------------------------------
This file is part of nsg-library.
http://nsg-library.googlecode.com/

Copyright (c) 2014-2015 Néstor Silveira Gorski

-------------------------------------------------------------------------------
This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
-------------------------------------------------------------------------------
*/
#include "GLES2IMGUI.h"
#include "GLES2RectangleMesh.h"
#include "GLES2CircleMesh.h"
#include "GLES2EllipseMesh.h"
#include "GLES2Camera.h"
#include "GLES2RoundedRectangleMesh.h"
#include "Node.h"
#include "Constants.h"
#include "Keys.h"
#include "Log.h"
#include "GLES2FrameColorSelection.h"
#include "GLES2Text.h"
#include "GLES2StencilMask.h"
#include "App.h"
#include <map>

using namespace NSG;

#define STRINGIFY(S) #S

static const char* vShader = STRINGIFY(
	uniform mat4 u_mvp;
	attribute vec4 a_position;
	varying vec4 v_position;
	
	void main()
	{
		gl_Position = u_mvp * vec4(a_position.xyz, 1);
		v_position = a_position;
	}
);

static const char* fShader = STRINGIFY(
	struct Material
	{
		vec4 diffuse;
	};
	uniform Material u_material;
	varying vec4 v_position;
	uniform int u_focus;

	void main()
	{
		float factor = 1.0;

		if(u_focus == 0)
		{
			factor = 1.0 - abs(v_position.y);
			gl_FragColor = u_material.diffuse * vec4(factor, factor, factor, 1);
		}
		else
		{
			gl_FragColor = vec4(u_material.diffuse.xyz, 1) * vec4(factor, factor, factor, 1);
		}

		
	}
);

namespace NSG 
{
	namespace IMGUI
	{
		PGLES2Camera pCamera;
		PGLES2RectangleMesh pRectangleMesh;
		PGLES2CircleMesh pCircleMesh;
		PGLES2EllipseMesh pEllipseMesh;
		PGLES2RoundedRectangleMesh pRoundedRectangle;

		ButtonType buttonType = RoundedRectangle;
		PGLES2Mesh pButtonMesh = pRoundedRectangle;

		PGLES2FrameColorSelection pFrameColorSelection;		

		PNode pCurrentNode(new Node());
        PNode pRenderNode(new Node(pCurrentNode));

		std::string currentFontFile("font/FreeSans.ttf");
		int currentFontSize = 18;
		size_t textMaxLength = std::numeric_limits<int>::max();

		bool fillEnabled = true;
		
		Color activeColor(0,1,0,0.7f);
		Color normalColor(0.5f,0.5f,0.5f,0.5f);
		Color hotColor(1,0.5f,0.5f,0.7f);
		Color borderColor(1,1,1,1);
		
		int tick = 0;
		int nestedLayout = 0;

		class TextManager
		{
		public:
			TextManager(const std::string& fontFile, int fontSize)
			: fontFile_(fontFile),
			fontSize_(fontSize)
			{
			}

			PGLES2Text GetTextMesh(GLushort item)
			{
				Key k(item);

				auto it = textMap_.find(k);

				if(it == textMap_.end())
				{
					PGLES2Text pTextMesh(new GLES2Text(fontFile_.c_str(), fontSize_, GL_STATIC_DRAW));

					textMap_.insert(TextMap::value_type(k, pTextMesh));

					return pTextMesh;
				}

				return it->second;
			}
		private:
			typedef GLushort Key;
			typedef std::map<Key, PGLES2Text> TextMap;
			TextMap textMap_;
			std::string fontFile_;
			int fontSize_;
		};

		typedef std::shared_ptr<TextManager> PTextManager;
		typedef std::pair<std::string, int> TextManagerKey;
		typedef std::map<TextManagerKey, PTextManager> TextManagerMap;
		TextManagerMap textManagers;

		PGLES2Text GetCurrentTextMesh(GLushort item)
		{
			TextManagerKey k(currentFontFile, currentFontSize);
			
			auto it = textManagers.find(k);

			if(it != textManagers.end())
			{
				return it->second->GetTextMesh(item);
			}
			else
			{
				PTextManager pTextManager(new TextManager(currentFontFile, currentFontSize));
				textManagers.insert(TextManagerMap::value_type(k, pTextManager));
				return pTextManager->GetTextMesh(item);
			}
		}

		struct UIState
		{
			float pixelSizeX;
			float pixelSizeY;

			float mousex;
			float mousey;
			bool mousedown;

			GLushort hotitem;
			GLushort activeitem;

			GLushort kbditem;
  			int keyentered;
  			int keymod;
  			int keyaction;
  			unsigned int character;
  			GLushort lastwidget;	
  			bool activeitem_is_a_text_field;
  			unsigned int cursor_character_position; 
		};

		UIState uistate = {0,0,0,0,false,0,0, 0,0,0,0,0,0, false, 0};

		void AllocateResources()
		{
			assert(pRectangleMesh == nullptr);
			PGLES2Program pProgram(new GLES2Program(vShader, fShader));
			PGLES2Material pMaterial(new GLES2Material());
            pMaterial->SetProgram(pProgram);
			pMaterial->SetDiffuseColor(Color(1,0,0,1));
			pRectangleMesh = PGLES2RectangleMesh(new GLES2RectangleMesh(1, 1, pMaterial, GL_STATIC_DRAW));
			pCircleMesh = PGLES2CircleMesh(new GLES2CircleMesh(0.5f, 64, pMaterial, GL_STATIC_DRAW));
			pEllipseMesh = PGLES2EllipseMesh(new GLES2EllipseMesh(1.0f, 1.0f, 64, pMaterial, GL_STATIC_DRAW));
			pRoundedRectangle = PGLES2RoundedRectangleMesh(new GLES2RoundedRectangleMesh(0.25f, 1, 0.5f, 64, pMaterial, GL_STATIC_DRAW));
            pCamera = PGLES2Camera(new GLES2Camera());
			pCamera->EnableOrtho();
            pCamera->SetFarClip(1);
            pCamera->SetNearClip(-1);
		}

		void ReleaseResources()
		{
            pCamera = nullptr;
			pRectangleMesh = nullptr;
			pCircleMesh = nullptr;
			pEllipseMesh = nullptr;
			pRoundedRectangle = nullptr;
			pButtonMesh = nullptr;
		}


		GLboolean isBlendEnabled = false;
		GLES2Camera* pActiveCamera = nullptr;
		void Begin()
		{
			isBlendEnabled = glIsEnabled(GL_BLEND);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	

			glDisable(GL_DEPTH_TEST);
			pActiveCamera = GLES2Camera::GetActiveCamera();
			pCamera->Activate();
			uistate.hotitem = 0;
		}

		void End()
		{
			if(pActiveCamera)
				pActiveCamera->Activate();

			if(!uistate.activeitem_is_a_text_field)
			{
				if(App::GetPtrInstance()->IsKeyboardVisible())
				{
					App::GetPtrInstance()->HideKeyboard();
					pCamera->SetPosition(Vertex3(0,0,0));
				}
			}
			else
			{
				if(!App::GetPtrInstance()->IsKeyboardVisible())
				{
					if(App::GetPtrInstance()->ShowKeyboard())
					{
						Vertex3 position(0, GetPosition().y - GetSize().y, 0);

			  			pCamera->SetPosition(ConvertScreenCoords2Pixels(position));
			  		}
			  	}
			}

			if(!uistate.mousedown)
			{
				uistate.activeitem = 0;
			}
			else
			{
				if(uistate.activeitem == 0)
				{
			  		uistate.activeitem = -1;
			  		uistate.activeitem_is_a_text_field = false;
			  	}
			}

			// If no widget grabbed tab, clear focus
			if (uistate.keyentered == NSG_KEY_TAB)
				uistate.kbditem = 0;
	
			// Clear the entered key
			uistate.keyentered = 0;	
			uistate.character = 0;		

			if(!isBlendEnabled)
				glDisable(GL_BLEND);
		}

		void DrawButton(Color color)
		{
            pButtonMesh->SetFilled(fillEnabled);
			pButtonMesh->GetMaterial()->SetDiffuseColor(color);
			pButtonMesh->Render(pRenderNode);
		}

		bool Hit(GLushort id)
		{
			if(!pFrameColorSelection)
				return false;

			pFrameColorSelection->Begin(uistate.mousex, uistate.mousey);
	    	pFrameColorSelection->Render(id, pButtonMesh, pRenderNode);
		    pFrameColorSelection->End();

		    return id == pFrameColorSelection->GetSelected();
		}

		void SetButtonType(ButtonType type)
		{
			buttonType = type;

			switch(buttonType)
			{
				case Rectangle:
					pButtonMesh = pRectangleMesh;
					break;
				case Circle:
					pButtonMesh = pCircleMesh;
					break;
				case Ellipse:
					pButtonMesh = pEllipseMesh;
					break;
				case RoundedRectangle:
					pButtonMesh = pRoundedRectangle;
					break;
				default:
					assert(false);
			}
		}

		Vertex3 ConvertPixels2ScreenCoords(const Vertex3& pixels)
		{
			Vertex3 screenCoords(pixels);
			screenCoords.x *= uistate.pixelSizeX;
			screenCoords.y *= uistate.pixelSizeY;
			return screenCoords;
		}

		Vertex3 ConvertScreenCoords2Pixels(const Vertex3& screenCoords)
		{
			Vertex3 pixels(screenCoords);
			pixels.x /= uistate.pixelSizeX;
			pixels.y /= uistate.pixelSizeY;
			return pixels;
		}

		void SetPosition(const Vertex3& position)
		{
			pCurrentNode->SetPosition(position);
		}

		const Vertex3& GetPosition()
		{
			return pCurrentNode->GetPosition();
		}

		void SetSize(const Vertex3& size)
		{
			pCurrentNode->SetScale(size);
		}

		const Vertex3& GetSize()
		{
			return pCurrentNode->GetScale();
		}

		void Fill(bool enable)
		{
			fillEnabled = enable;
		}

		void SetBorderColor(Color color)
		{
			borderColor = color;
		}

		void SetNormalColor(Color color)
		{
			normalColor = color;
		}

		void SetHotColor(Color color)
		{
			hotColor = color;
		}

		void SetActiveColor(Color color)
		{
			activeColor = color;
		}

		void SetFont(const std::string& fontFile, int fontSize)
		{
			currentFontFile = fontFile;
			currentFontSize = fontSize;
		}

		void SetTextMaxLength(size_t maxLength)
		{
			textMaxLength = maxLength;
		}

		void Try2Add2Layout(GLushort id)
		{
			//TODO
		}

		Vertex3 GetLayoutPositionFor(GLushort id)
		{
			//TODO
			return Vertex3(0,0,0);
		}

		Vertex3 GetLayoutSizeFor(GLushort id)
		{
			//TODO
			return Vertex3(0,0,0);
		}

		void BeginHorizontal()
		{
			++nestedLayout;
		}

		void EndHorizontal()
		{
			--nestedLayout;
		}

		void BeginVertical()
		{
			++nestedLayout;
		}

		void EndVertical()
		{
			--nestedLayout;
		}

		bool InternalButton(GLushort id, const std::string& text)
		{
			Vertex3 currentPosition = GetPosition();
			Vertex3 currentSize = GetSize();

			if(nestedLayout)
			{
				Try2Add2Layout(id);
				SetPosition(GetLayoutPositionFor(id));
				SetSize(GetLayoutSizeFor(id));
			}

			// Check whether the button should be hot
			if (Hit(id))
			{
				uistate.hotitem = id;

				if (uistate.activeitem == 0 && uistate.mousedown)
				{
					uistate.activeitem_is_a_text_field = false;
			  		uistate.activeitem = id;
			  		uistate.kbditem = id;
			  	}
			}

			// If no widget has keyboard focus, take it
			if (uistate.kbditem == 0)
				uistate.kbditem = id;

			// If we have keyboard focus, show it
			if (uistate.kbditem == id)
				pButtonMesh->GetMaterial()->SetUniform("u_focus", 1);
			else
				pButtonMesh->GetMaterial()->SetUniform("u_focus", 0);


			if(uistate.hotitem == id)
			{
				if(uistate.activeitem == id)
				{
			  		// Button is both 'hot' and 'active'
			  		DrawButton(activeColor);
			  	}
				else
				{
					// Button is merely 'hot'
					DrawButton(hotColor);
				}
			}
			else
			{
				// button is not hot, but it may be active    
				DrawButton(normalColor);
			}

			if(fillEnabled)
			{
				fillEnabled = false;
				DrawButton(borderColor);
				fillEnabled = true;
			}

			{
				GLES2StencilMask stencilMask;
				stencilMask.Begin();
				stencilMask.Render(pRenderNode.get(), pButtonMesh.get());
				stencilMask.End();
				PGLES2Text pTextMesh = GetCurrentTextMesh(id);
	            pTextMesh->SetText(text);
				
				static PNode pRootTextNode(new Node());
				pRootTextNode->SetPosition(Vertex3(-pTextMesh->GetWidth()/2, -pTextMesh->GetHeight()/2, 0));
				pCurrentNode->SetParent(pRootTextNode);
				Vertex3 scale = pCurrentNode->GetScale();
				pCurrentNode->SetScale(Vertex3(1,1,1));
				pTextMesh->Render(pRenderNode, Color(1,1,1,1));
				pCurrentNode->SetScale(scale);
                pCurrentNode->SetParent(nullptr);
			}

			bool enterKey = false;

			// If we have keyboard focus, we'll need to process the keys
			if (uistate.kbditem == id)
			{
				switch (uistate.keyentered)
				{
				case NSG_KEY_TAB:
					// If tab is pressed, lose keyboard focus.
					// Next widget will grab the focus.
					uistate.kbditem = 0;
					// If shift was also pressed, we want to move focus
					// to the previous widget instead.
					if (uistate.keymod & NSG_KEY_MOD_SHIFT)
						uistate.kbditem = uistate.lastwidget;
					// Also clear the key so that next widget
					// won't process it
					uistate.keyentered = 0;
					break;

				case NSG_KEY_ENTER:
					// Had keyboard focus, received return,
					// so we'll act as if we were clicked.
					enterKey = true;
				  	break;
				}
			}

			uistate.lastwidget = id;

			if(nestedLayout)
			{
				//Restore user position and size
				SetPosition(currentPosition);
				SetSize(currentSize);
			}

			// If button is hot and active, but mouse button is not
			// down, the user must have clicked the button.
			return enterKey || (uistate.mousedown == 0 && uistate.hotitem == id &&  uistate.activeitem == id);
		}		

		std::string InternalTextField(GLushort id, const std::string& text, std::regex* pRegex)
		{
			Vertex3 currentPosition = GetPosition();
			Vertex3 currentSize = GetSize();

			if(nestedLayout)
			{
				Try2Add2Layout(id);
				SetPosition(GetLayoutPositionFor(id));
				SetSize(GetLayoutSizeFor(id));
			}

			std::string currentText = text;

			// Check whether the button should be hot
			if (Hit(id))
			{
				uistate.hotitem = id;

				if (uistate.activeitem == 0 && uistate.mousedown)
				{
			  		uistate.activeitem = id;
			  		uistate.kbditem = id;
			  		uistate.activeitem_is_a_text_field = true;
                    uistate.cursor_character_position = currentText.length();
			  	}
			}

			// If no widget has keyboard focus, take it
			if (uistate.kbditem == 0)
				uistate.kbditem = id;

			// If we have keyboard focus, show it
			if (uistate.kbditem == id)
				pButtonMesh->GetMaterial()->SetUniform("u_focus", 1);
			else
				pButtonMesh->GetMaterial()->SetUniform("u_focus", 0);


			if(uistate.hotitem == id)
			{
				if(uistate.activeitem == id)
				{
			  		// Button is both 'hot' and 'active'
			  		//DrawButton(activeColor);
                    DrawButton(hotColor);
			  	}
				else
				{
					// Button is merely 'hot'
					DrawButton(hotColor);
				}
			}
			else
			{
				// button is not hot, but it may be active    
				DrawButton(normalColor);
			}

			if(fillEnabled)
			{
				fillEnabled = false;
				DrawButton(borderColor);
				fillEnabled = true;
			}

			{
				GLES2StencilMask stencilMask;
				stencilMask.Begin();
				stencilMask.Render(pRenderNode.get(), pButtonMesh.get());
				stencilMask.End();
				PGLES2Text pTextMesh = GetCurrentTextMesh(id);
				PGLES2Text pCursorMesh = GetCurrentTextMesh(-1);
				pCursorMesh->SetText("|");

	            pTextMesh->SetText(currentText);
				
				static PNode pRootTextNode(new Node());
				float xPos = -GetSize().x/2;
				float yPos = 0;

				if(GetSize().x < pTextMesh->GetWidth())
				{
					xPos -= pCursorMesh->GetWidth() + pTextMesh->GetWidth() - GetSize().x;  
				}

				pRootTextNode->SetPosition(Vertex3(xPos, yPos, 0));
				pCurrentNode->SetParent(pRootTextNode);
				Vertex3 scale = pCurrentNode->GetScale();
				pCurrentNode->SetScale(Vertex3(1,1,1));
				pTextMesh->Render(pRenderNode, Color(1,1,1,1));

				// Render cursor if we have keyboard focus
				if(uistate.kbditem == id && (tick < 15))
                {
                	xPos += pTextMesh->GetWidthForCharacterPosition(uistate.cursor_character_position);
                	pRootTextNode->SetPosition(Vertex3(xPos, yPos, 0));
					pCursorMesh->Render(pRenderNode, Color(1,0,0,1));
                }

				pCurrentNode->SetScale(scale);
                pCurrentNode->SetParent(nullptr);
			}

			// If we have keyboard focus, we'll need to process the keys
			if (uistate.kbditem == id)
			{
				switch (uistate.keyentered)
				{
				case NSG_KEY_TAB:
					// If tab is pressed, lose keyboard focus.
					// Next widget will grab the focus.
					uistate.kbditem = 0;
					// If shift was also pressed, we want to move focus
					// to the previous widget instead.
					if (uistate.keymod & NSG_KEY_MOD_SHIFT)
						uistate.kbditem = uistate.lastwidget;
					// Also clear the key so that next widget
					// won't process it
					uistate.keyentered = 0;
					break;
				case NSG_KEY_BACKSPACE:
					if(uistate.cursor_character_position > 0)
					{
                        std::string::iterator it = currentText.begin() + uistate.cursor_character_position - 1;
                        currentText.erase(it);
                        --uistate.cursor_character_position;
					}
					break;   

                case NSG_KEY_DELETE:
					if(uistate.cursor_character_position < currentText.length())
					{
                        std::string::iterator it = currentText.begin() + uistate.cursor_character_position;
                        currentText.erase(it);
					}
                    break;

                case NSG_KEY_RIGHT:
                    if(uistate.cursor_character_position < currentText.length())
                        ++uistate.cursor_character_position;
                    break;

                case NSG_KEY_LEFT:
                    if(uistate.cursor_character_position > 0)
                        --uistate.cursor_character_position;
                    break;

                case NSG_KEY_HOME:
                    uistate.cursor_character_position = 0;
                    break;

                case NSG_KEY_END:
                    uistate.cursor_character_position = currentText.length();
                    break;
				}

	            if (uistate.character >= 32 && uistate.character < 127 && currentText.size() < textMaxLength)
	            {
	            	std::string textCopy = currentText;

                    std::string::iterator it = currentText.begin() + uistate.cursor_character_position;
	                currentText.insert(it, (char)uistate.character);
                    ++uistate.cursor_character_position;

	            	if(pRegex)
	            	{
	            		if(!regex_match(currentText, *pRegex))
	            		{
	            			currentText = textCopy;
	            			--uistate.cursor_character_position;
	            		}
	            	}
	            }
			}

			// If button is hot and active, but mouse button is not
			// down, the user must have clicked the widget; give it 
			// keyboard focus.
			if (uistate.mousedown == 0 && uistate.hotitem == id && uistate.activeitem == id)
				uistate.kbditem = id;

			uistate.lastwidget = id;

			if(nestedLayout)
			{
				//Restore user position and size
				SetPosition(currentPosition);
				SetSize(currentSize);
			}

			return currentText;
		}		


		void ViewChanged(int32_t width, int32_t height)
		{
			//////////////////////////////////////////////////////////////////////////////////////////////////
			//Since we are using screen coordinates then  we have to recalculate the origin of coordinates 
			//and the scale to match the ortographic projection
			Vertex3 coordinates_origin(width/2, height/2, 0);
			Vertex3 coordinates_scale(width/2, height/2, 1);

            pRenderNode->SetPosition(coordinates_origin);
            pRenderNode->SetScale(coordinates_scale);
            //////////////////////////////////////////////////////////////////////////////////////////////////
            
			uistate.pixelSizeX = 2/(float)width;
			uistate.pixelSizeY = 2/(float)height;

			if(!pFrameColorSelection)
        		pFrameColorSelection = PGLES2FrameColorSelection(new GLES2FrameColorSelection());

        	pFrameColorSelection->ViewChanged(width, height);
		}

        void OnMouseMove(float x, float y)
        {
        	uistate.mousex = x;
        	uistate.mousey = y;
        }

        void OnMouseDown(float x, float y)
        {
        	uistate.mousex = x;
        	uistate.mousey = y;
        	uistate.mousedown = true;
        }

        void OnMouseUp()
        {
        	uistate.mousedown = false;
        }

        void OnKey(int key, int action, int modifier)
        {
            if(action == NSG_KEY_PRESS)
            {
                uistate.keyentered = key;
                uistate.keyaction = action;
                uistate.keymod = modifier;
            }
        }

        void OnChar(unsigned int character)
        {
        	uistate.character = character;
        }


        void DoTick(float delta)
        {
        	++tick;
            if(tick > 30)
                tick = 0;
        }
	}
}

