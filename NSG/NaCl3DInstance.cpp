#ifdef NACL
#include "NaCl3DInstance.h"

#include "sys/mount.h"
#include "nacl_io/nacl_io.h"
#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_array.h"
#include "ppapi/lib/gl/gles2/gl2ext_ppapi.h"
#include "Log.h"

static pp::Instance* s_ppInstance = nullptr;

int PPPrintMessage(const char* format, ...) 
{
	va_list args;
	const int maxBuffer = 1024;
	char buffer[maxBuffer];
	va_start(args, format);
	int ret_status = vsnprintf(buffer, maxBuffer, format, args);
	va_end(args);

	assert(s_ppInstance != nullptr && "s_ppInstance is null");

	pp::Var var_reply(buffer);
	s_ppInstance->PostMessage(var_reply);

	return ret_status;	
}

static pthread_t g_handle_message_thread;

namespace NSG 
{
	namespace NaCl 
	{
		NaCl3DInstance::NaCl3DInstance(PP_Instance instance, PApp pApp) 
		: pp::Instance(instance), callback_factory_(this), pApp_(new InternalApp(pApp)), width_(0), height_(0)
		{
			s_ppInstance = this;

			RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE | PP_INPUTEVENT_CLASS_WHEEL | PP_INPUTEVENT_CLASS_TOUCH);
    		RequestFilteringInputEvents(PP_INPUTEVENT_CLASS_KEYBOARD);			
		}

		pp::Instance* NaCl3DInstance::GetInstance()
		{
			return s_ppInstance;
		}

		bool NaCl3DInstance::Init(uint32_t argc, const char* argn[], const char* argv[]) 
		{
			nacl_io_init_ppapi(pp::Instance::pp_instance(), pp::Module::Get()->get_browser_interface());

			int status = mount("","/persistence","html5fs",0,"type=PERSISTENT,expected_size=1048576");

			if(status != 0)
				TRACE_LOG("mount has failed with error=" << status)

			return true;
		}

		void NaCl3DInstance::DidChangeView(const pp::View& view) 
		{
			width_ = view.GetRect().width();
			height_ = view.GetRect().height();
			if (context_.is_null()) 
			{
				if (!InitGL(width_, height_)) 
				{
					TRACE_LOG("InitGL has failed!")
					return; // failed.
				}
				pApp_->Initialize(30);
				pApp_->ViewChanged(width_, height_);
				MainLoop(0);
			} 
			else 
			{
				// Resize the buffers to the new size of the module.
				int32_t result = context_.ResizeBuffers(width_, height_);
				if (result < 0) {
					fprintf(stderr,
							"Unable to resize buffers to %d x %d!\n",
							width_,
							height_);

					return;
				}
			}

			pApp_->ViewChanged(width_, height_);
		}

		void NaCl3DInstance::HandleMessage(const pp::Var& message) 
		{
			pApp_->HandleMessage(message);
		}

		bool NaCl3DInstance::HandleInputEvent(const pp::InputEvent& event) 
		{
			switch (event.GetType()) 
			{
				case PP_INPUTEVENT_TYPE_IME_COMPOSITION_START:
				case PP_INPUTEVENT_TYPE_IME_COMPOSITION_UPDATE:
				case PP_INPUTEVENT_TYPE_IME_COMPOSITION_END:
				case PP_INPUTEVENT_TYPE_IME_TEXT:
				case PP_INPUTEVENT_TYPE_UNDEFINED:
					// these cases are not handled.
					break;

				case PP_INPUTEVENT_TYPE_MOUSEMOVE:
				{
					if(height_ > 0 && width_ > 0)
					{
						pp::MouseInputEvent mouse_event(event);
						pApp_->OnMouseMove(-1 + 2*(double)mouse_event.GetPosition().x()/width_, 1 + -2*(double)mouse_event.GetPosition().y()/height_);
					}
					break;
				}


				case PP_INPUTEVENT_TYPE_MOUSEDOWN:
				{
					if(height_ > 0 && width_ > 0)
					{
						pp::MouseInputEvent mouse_event(event);

						pApp_->OnMouseDown(-1 + 2*(double)mouse_event.GetPosition().x()/width_, 1 + -2*(double)mouse_event.GetPosition().y()/height_);
					}
					break;
				}

				case PP_INPUTEVENT_TYPE_MOUSEUP:
				{
					pApp_->OnMouseUp();
					break;
				}

				case PP_INPUTEVENT_TYPE_MOUSEENTER:
				case PP_INPUTEVENT_TYPE_MOUSELEAVE:
				case PP_INPUTEVENT_TYPE_CONTEXTMENU: 
				{
					pp::MouseInputEvent mouse_event(event);

					/*std::ostringstream stream;
					stream << "Mouse event:"
					       << " modifier:" << ModifierToString(mouse_event.GetModifiers())
					       << " button:" << MouseButtonToString(mouse_event.GetButton())
					       << " x:" << mouse_event.GetPosition().x()
					       << " y:" << mouse_event.GetPosition().y()
					       << " click_count:" << mouse_event.GetClickCount()
					       << " time:" << mouse_event.GetTimeStamp()
					       << " is_context_menu: "
					           << (event.GetType() == PP_INPUTEVENT_TYPE_CONTEXTMENU);
					PostMessage(stream.str());*/
					break;
				}

				case PP_INPUTEVENT_TYPE_WHEEL: 
				{
					pp::WheelInputEvent wheel_event(event);
					/*
					std::ostringstream stream;
					stream << "Wheel event:"
					       << " modifier:" << ModifierToString(wheel_event.GetModifiers())
					       << " deltax:" << wheel_event.GetDelta().x()
					       << " deltay:" << wheel_event.GetDelta().y()
					       << " wheel_ticks_x:" << wheel_event.GetTicks().x()
					       << " wheel_ticks_y:" << wheel_event.GetTicks().y()
					       << " scroll_by_page: " << wheel_event.GetScrollByPage()
					       << " time:" << wheel_event.GetTimeStamp();
					PostMessage(stream.str());*/
					break;
				}

				case PP_INPUTEVENT_TYPE_RAWKEYDOWN:
				case PP_INPUTEVENT_TYPE_KEYDOWN:
				case PP_INPUTEVENT_TYPE_KEYUP:
				case PP_INPUTEVENT_TYPE_CHAR: 
				{
					pp::KeyboardInputEvent key_event(event);
					/*std::ostringstream stream;
					stream << "Key event:"
					       << " modifier:" << ModifierToString(key_event.GetModifiers())
					       << " key_code:" << key_event.GetKeyCode()
					       << " time:" << key_event.GetTimeStamp()
					       << " text:" << key_event.GetCharacterText().DebugString();
					PostMessage(stream.str());*/
					break;
				}

				case PP_INPUTEVENT_TYPE_TOUCHSTART:
				case PP_INPUTEVENT_TYPE_TOUCHMOVE:
				case PP_INPUTEVENT_TYPE_TOUCHEND:
				case PP_INPUTEVENT_TYPE_TOUCHCANCEL: 
				{
					pp::TouchInputEvent touch_event(event);
					/*std::ostringstream stream;
					stream << "Touch event:" << TouchKindToString(event.GetType())
					       << " modifier:" << ModifierToString(touch_event.GetModifiers());

					uint32_t touch_count =
					    touch_event.GetTouchCount(PP_TOUCHLIST_TYPE_CHANGEDTOUCHES);
					for (uint32_t i = 0; i < touch_count; ++i) {
					  pp::TouchPoint point =
					      touch_event.GetTouchByIndex(PP_TOUCHLIST_TYPE_CHANGEDTOUCHES, i);
					  stream << " x[" << point.id() << "]:" << point.position().x()
					         << " y[" << point.id() << "]:" << point.position().y()
					         << " radii_x[" << point.id() << "]:" << point.radii().x()
					         << " radii_y[" << point.id() << "]:" << point.radii().y()
					         << " angle[" << point.id() << "]:" << point.rotation_angle()
					         << " pressure[" << point.id() << "]:" << point.pressure();
					}
					stream << " time:" << touch_event.GetTimeStamp();
					PostMessage(stream.str());*/
					break;
				}

				default: 
				{
					// For any unhandled events, send a message to the browser
					// so that the user is aware of these and can investigate.
					/*std::stringstream oss;
					oss << "Default (unhandled) event, type=" << event.GetType();
					PostMessage(oss.str());*/
					break;
				} 
			}

			return true;
		}


		bool NaCl3DInstance::InitGL(int32_t new_width, int32_t new_height) 
		{
			if (!glInitializePPAPI(pp::Module::Get()->get_browser_interface())) 
			{
				fprintf(stderr, "Unable to initialize GL PPAPI!\n");
				return false;
			}
			const int32_t attrib_list[] = 
			{
				PP_GRAPHICS3DATTRIB_STENCIL_SIZE, 8,
				PP_GRAPHICS3DATTRIB_ALPHA_SIZE, 8,
				PP_GRAPHICS3DATTRIB_DEPTH_SIZE, 24,
				PP_GRAPHICS3DATTRIB_WIDTH, new_width,
				PP_GRAPHICS3DATTRIB_HEIGHT, new_height,
				PP_GRAPHICS3DATTRIB_NONE
			};
			
			context_ = pp::Graphics3D(this, attrib_list);
			
			if (!BindGraphics(context_)) 
			{
				fprintf(stderr, "Unable to bind 3d context!\n");
				context_ = pp::Graphics3D();
				glSetCurrentContextPPAPI(0);
				return false;
			}

			glSetCurrentContextPPAPI(context_.pp_resource());
			
			return true;
		}

		void NaCl3DInstance::MainLoop(int32_t) 
		{
			pApp_->PerformTick();
			pApp_->RenderFrame();		
			context_.SwapBuffers(callback_factory_.NewCallback(&NaCl3DInstance::MainLoop));
		}

		Graphics3DModule::Graphics3DModule(NSG::PApp pApp) : pp::Module(), pApp_(pApp) 
		{
		}

		Graphics3DModule::~Graphics3DModule() 
		{
		}

		pp::Instance* Graphics3DModule::CreateInstance(PP_Instance instance) 
		{
			return new NaCl3DInstance(instance, pApp_);
		}
	}
}
#endif